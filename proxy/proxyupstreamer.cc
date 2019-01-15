#include <system_error>
#include <socks6util/socks6util.hh>
#include "../core/poller.hh"
#include "proxy.hh"
#include "authserver.hh"
#include "connectproxydownstreamer.hh"
#include "simpleproxydownstreamer.hh"
#include "proxyupstreamer.hh"

using namespace std;
using namespace boost;

void ProxyUpstreamer::honorRequest()
{
	try
	{
		if (mustFail)
			throw SimpleReplyException(SOCKS6_OPERATION_REPLY_FAILURE);
		
		switch (request->getCommandCode())
		{
		case SOCKS6_REQUEST_CONNECT:
		{
			honorConnect();
			break;
		}
		case SOCKS6_REQUEST_NOOP:
		{
			throw SimpleReplyException(SOCKS6_OPERATION_REPLY_SUCCESS);
		}
		default:
		{
			throw SimpleReplyException(SOCKS6_OPERATION_REPLY_CMD_NOT_SUPPORTED);
		}
		}
	}
	catch (SimpleReplyException &ex)
	{
		S6M::OperationReply reply(ex.getCode(), S6M::Address(S6U::Socket::QUAD_ZERO), 0);
		*reply.getOptionSet() = replyOptions;
		poller->assign(new SimpleProxyDownstreamer(this, &reply));
	}
	catch (std::exception &)
	{
		S6M::OperationReply reply(SOCKS6_OPERATION_REPLY_FAILURE, S6M::Address(S6U::Socket::QUAD_ZERO), 0);
		*reply.getOptionSet() = replyOptions;
		poller->assign(new SimpleProxyDownstreamer(this, &reply));
	}
}

void ProxyUpstreamer::honorConnect()
{
	//TODO: resolve
	if (request->getAddress()->getType() == SOCKS6_ADDR_DOMAIN)
		throw SimpleReplyException(SOCKS6_OPERATION_REPLY_ADDR_NOT_SUPPORTED);

	S6U::SocketAddress addr(*request->getAddress(), request->getPort());
	dstSock.fd.assign(socket(addr.sockAddress.sa_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP));
	if (dstSock.fd < 0)
		throw system_error(errno, system_category());

	honorConnectStackOptions();

	try
	{
		dstSock.sockConnect(addr, &buf, request->getOptionSet()->getTFOPayload(), false);
	}
	catch (system_error &err)
	{
		throw SimpleReplyException(S6U::Socket::connectErrnoToReplyCode(err.code().value()));
	}

	dstSock.clientHandshake();

	poller->add(this, dstSock.fd, Poller::OUT_EVENTS);
	state = S_CONNECTING;
}

void ProxyUpstreamer::honorConnectStackOptions()
{
	SOCKS6MPTCPScheduler clientProxySched = request->getOptionSet()->getClientProxySched();
	if (S6U::Socket::setMPTCPSched(srcSock.fd, clientProxySched) == 0)
		replyOptions.setClientProxySched(clientProxySched);

	SOCKS6MPTCPScheduler proxyServerSched = request->getOptionSet()->getProxyRemoteSched();
	if (S6U::Socket::setMPTCPSched(dstSock.fd, proxyServerSched) == 0)
		replyOptions.setProxyRemoteSched(proxyServerSched);
}

ProxyUpstreamer::ProxyUpstreamer(Proxy *proxy, int *pSrcFD, TLSContext *serverCtx)
	: StreamReactor(proxy->getPoller()), proxy(proxy), state(S_HANDSHAKE), authenticated(false), replyOptions(S6M::OptionSet::M_OP_REP), authServer(NULL), mustFail(false)
{
	srcSock.fd.assign(*pSrcFD);
	*pSrcFD = -1;
	
	if (serverCtx != NULL)
		srcSock.tls = new TLS(serverCtx, srcSock.fd);
}

void ProxyUpstreamer::start()
{
	process(-1, 0);
}

void ProxyUpstreamer::process(int fd, uint32_t events)
{
	bool fellThrough = false;
	
	switch (state)
	{
	case S_HANDSHAKE:
	{
		srcSock.serverHandshake(&buf);
		
		state = S_READING_REQ;
		fellThrough = true;
		[[fallthrough]];
	}
		
	case S_READING_REQ:
	{
		if (!fellThrough || buf.usedSize() == 0)
		{
			ssize_t bytes = srcSock.sockRecv(&buf);
			if (bytes == 0)
				return;
		}

		S6M::ByteBuffer bb(buf.getHead(), buf.usedSize());
		try
		{
			request = std::shared_ptr<S6M::Request>(new S6M::Request(&bb));
			buf.unuse(bb.getUsed());
		}
		catch (S6M::BadVersionException &)
		{
			SOCKS6Version version = { SOCKS6_VERSION_MAJOR, SOCKS6_VERSION_MINOR };
			poller->assign(new SimpleProxyDownstreamer(this, &version));
			return;
		}
		catch (S6M::EndOfBufferException &)
		{
			poller->add(this, srcSock.fd, Poller::IN_EVENTS);
			return;
		}

		poller->assign(new AuthServer(this));

		tfoPayload = std::min((size_t)request->getOptionSet()->getTFOPayload(), MSS);
		
		if (buf.usedSize() < tfoPayload)
		{
			state = S_READING_TFO_PAYLOAD;
			poller->add(this, srcSock.fd, Poller::IN_EVENTS);
		}
		else
		{
			honorLock.acquire();
			state = S_AWAITING_AUTH;
			bool honor = authenticated;
			honorLock.release();
			
			if (honor)
				honorRequest();
		}
		break;
	}
	case S_AWAITING_AUTH:
	{
		// this point shouldn't be reached
		break;
	}
	case S_READING_TFO_PAYLOAD:
	{
		ssize_t bytes = srcSock.sockRecv(&buf);
		if (bytes == 0)
		{
			deactivate();
			return;
		}
		if (buf.usedSize() >= tfoPayload)
		{
			honorLock.acquire();
			state = S_AWAITING_AUTH;
			bool honor = authenticated;
			honorLock.release();
			
			if (honor)
				honorRequest();
		}
		break;
	}
	case S_CONNECTING:
	{
		SOCKS6OperationReplyCode code;

		int err;
		socklen_t errLen = sizeof(err);
		int rc = getsockopt(dstSock.fd, SOL_SOCKET, SO_ERROR, &err, &errLen);
		if (rc < 0)
			code = SOCKS6_OPERATION_REPLY_FAILURE;
		else
			code = S6U::Socket::connectErrnoToReplyCode(err);

		if (code != SOCKS6_OPERATION_REPLY_SUCCESS)
		{
			S6M::OperationReply reply(code, S6M::Address(S6U::Socket::QUAD_ZERO), 0);
			*reply.getOptionSet() = replyOptions;
			poller->assign(new SimpleProxyDownstreamer(this, &reply));
			return;
		}

		S6U::SocketAddress bindAddr;
		socklen_t addrLen = sizeof(bindAddr.storage);
		rc = getsockname(dstSock.fd, &bindAddr.sockAddress, &addrLen);
		if (rc < 0)
			throw system_error(errno, system_category());

		if (S6U::Socket::hasMPTCP(dstSock.fd) > 0)
			replyOptions.setMPTCP();

		S6M::OperationReply reply(SOCKS6_OPERATION_REPLY_SUCCESS, bindAddr.getAddress(), bindAddr.getPort());
		*reply.getOptionSet() = replyOptions;
		poller->assign(new ConnectProxyDownstreamer(this, &reply));

		state = S_STREAM;
		if (buf.usedSize() > 0)
			streamState = SS_SENDING;
		else
			streamState = SS_RECEIVING;
		[[fallthrough]];
	}
	case S_STREAM:
	{
		StreamReactor::process(fd, events);
		break;
	}
	}
}

void ProxyUpstreamer::authDone(SOCKS6TokenExpenditureCode expenditureCode)
{
	if (expenditureCode != (SOCKS6TokenExpenditureCode)0)
		replyOptions.setExpenditureReply(expenditureCode);
	
	honorLock.acquire();
	authenticated = true;
	bool honor = state == S_AWAITING_AUTH;
	honorLock.release();
	
	if (honor)
		honorRequest();
}
