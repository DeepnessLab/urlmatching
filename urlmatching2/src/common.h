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

#else
//Only valid for unix systems
inline
int get_curr_memsize() { return 0; }
#endif




#endif /* COMMON_H_ */
