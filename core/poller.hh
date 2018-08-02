#ifndef POLLER_HH
#define POLLER_HH

#include <unordered_map>
#include <thread>
#include <vector>
#include <sys/epoll.h>
#include <boost/thread/mutex.hpp>
#include <exception>
#include "reactor.hh"

class Poller
{
	struct FDEntry
	{
		boost::intrusive_ptr<Reactor> reactor;
		bool registered;
		
		FDEntry()
			: registered(false) {}
	};
	
	int numThreads;
	
	int epollFD;
	
	std::vector<std::thread> threads;
	
	std::vector<FDEntry> fdEntries;
	
	volatile bool alive;
	
public:
	static const uint32_t IN_EVENTS  = EPOLLIN | EPOLLRDHUP;
	static const uint32_t OUT_EVENTS = EPOLLOUT;
	
	Poller(int numThreads, int cpuOffset, size_t expectedFDs = 1 << 17);
	
	void add(boost::intrusive_ptr<Reactor> reactor, int fd, uint32_t events);
	
	void remove(int fd, bool force = false);
	
	void stop();
	
	void join();
	
	static void threadFun(Poller *poller);

	class MaxFDsExceededException: public std::exception
	{
	public:
		const char *what() const throw();
	};
};

#endif // POLLER_HH
