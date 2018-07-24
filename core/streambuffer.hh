#ifndef STREAMBUFFER_HH
#define STREAMBUFFER_HH

#include <stdint.h>
#include <unistd.h>

class StreamBuffer
{
	static const size_t BUF_SIZE = 100 * 1024; //100KB
	
	uint8_t buf[BUF_SIZE];
	
	size_t head;
	size_t tail;
	
public:
	StreamBuffer()
		: head(0), tail(0) {}
	
	uint8_t *getHead()
	{
		return &buf[head];
	}
	
	size_t usedSize() const
	{
		return tail - head;
	}
	
	void unuseHead(size_t count)
	{
		head += count;
		if (head == tail)
		{
			head = 0;
			tail = 0;
		}
	}

	void unuseTail(size_t count)
	{
		tail -= count;
		if (head == tail)
		{
			head = 0;
			tail = 0;
		}
	}
	
	uint8_t *getTail()
	{
		return &buf[tail];
	}
	
	size_t availSize() const
	{
		return BUF_SIZE - tail;
	}
	
	void use(size_t count)
	{
		tail += count;
	}
};

#endif // STREAMBUFFER_HH