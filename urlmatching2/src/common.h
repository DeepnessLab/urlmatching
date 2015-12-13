/*
 * common.h
 *
 *  Created on: 18 December 2014
 *      Author: Daniel Krauthgamer
 *
 * Common defines
 *
 */

#ifndef COMMON_H_
#define COMMON_H_

#define DELETE_AND_NULL(ptr) \
		{ delete ptr ;\
		ptr = NULL; } while (0)

inline double Byte2KB(uint32_t bytes) {
	return (double((double)bytes / 1024));
}

#ifdef __unix__
#include <malloc.h>
inline
int get_curr_memsize()
{
	struct mallinfo mi;
	mi = mallinfo();
	int mem = mi.uordblks;
	return mem;
}

#else	//windows os
//Only valid for unix systems
inline
int get_curr_memsize() { return 0; }
#endif


#define GETTIMING double(end - begin) / (CLOCKS_PER_SEC)
#define START_TIMING 	do {begin = std::clock();} while(0)
#define STOP_TIMING 	do {end = std::clock();} while(0)
#define PREPARE_TIMING clock_t begin,end

/*
 * Unix/Windows
 * Timer:
 */
class Timer {
public:
	Timer(bool start=true):
		_begin(0),_time(0l),_running(false)
	{
		if (start)
			start();
	}
	virtual ~Timer() {}

	void reset()	{
		_time = 0;
		_begin = 0;
		_running = false;
	}
	void start() 	{
		_begin = std::clock();
		_running = true;
	}
	void stop()		{
		clock_t end = std::clock();
		if (_running) {
			_time+= (end - _begin);
			_running = false;
		}
	}
	// Stop and get_seconds
	double get_seconds()	{
		stop();
		return double ( double (_time) / (CLOCKS_PER_SEC) );
	}

private:
	clock_t _begin;
	clock_t _time;
	bool _running;
};



#endif /* COMMON_H_ */
