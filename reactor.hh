#ifndef REACTOR_HH
#define REACTOR_HH

#include <stdint.h>
#include <atomic>

class Poller;

class Reactor
{
protected:
	bool alive;
	std::atomic<int> refCnt;
	int fd;
	
public:
	Reactor(int fd)
		: alive(true), refCnt(0), fd(fd) {}
	
	virtual void process(Poller *poller, uint32_t events) = 0;
	
	void use()
	{
		refCnt++;
	}

	void unuse()
	{
		refCnt--;
		if (refCnt == 0)
			delete this;
	}
	
	void kill()
	{
		alive = false;
	}
	
	int getFD() const
	{
		return fd;
	}
	
	virtual ~Reactor();
};

#endif // REACTOR_HH
