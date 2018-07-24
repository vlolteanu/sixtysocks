#ifndef WINDOWSUPPLICANT_HH
#define WINDOWSUPPLICANT_HH

#include "../core/reactor.hh"
#include "../core/streambuffer.hh"
#include "../core/uniqfd.hh"

class Proxifier;

class WindowSupplicant: public Reactor
{
	enum State
	{
		S_SENDING_REQ,
		S_RECEIVING_AUTH_REP,
	};
	
	boost::intrusive_ptr<Proxifier> proxifier;
	
	int fd;
	
	StreamBuffer buf;
	
	State state;
	
	bool supplicating;
	
public:
	WindowSupplicant(Proxifier *proxifier);
	
	~WindowSupplicant();
	
	void process(int fd, uint32_t events);
	
	void deactivate();
	
	void start(bool defer);
};

#endif // WINDOWSUPPLICANT_HH