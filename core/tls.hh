#ifndef TLS_HH
#define TLS_HH

#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <socks6util/socks6util.hh>
#include "tlscontext.hh"
#include "streambuffer.hh"

class TLS: public boost::intrusive_ref_counter<TLS>
{
	TLSContext *ctx;

	int rfd;
	int wfd;

	WOLFSSL *readTLS;
	WOLFSSL *writeTLS;

	bool connected;
	
public:
	TLS(TLSContext *ctx, int fd);
	
	~TLS();
	
	void setReadFD(int fd);
	
	void setWriteFD(int fd);
	
	void tlsConnect(S6U::SocketAddress *addr, StreamBuffer *buf, bool useEarlyData);
	
	void tlsAccept(StreamBuffer *buf);
	
	size_t tlsWrite(StreamBuffer *buf);
	
	size_t tlsRead(StreamBuffer *buf);
};

#endif // TLS_HH