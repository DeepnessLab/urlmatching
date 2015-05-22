/*
 * logger.h
 *
 *  Created on: 9 ���� 2015
 *      Author: Daniel
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <stdio.h>
#include "easylogging++.h"

#ifdef BUILD_DEBUG
#define ASSERT(what) if (!(what)) { \
	el::Loggers::flushAll();\
	assert(what); }
#else
#define ASSERT(what) do {} while(0)
#endif

#define STR(s) #s
#define XSTR(a) STR(a)

#define DEBUG_NONE 0
#define DEBUG_LOG 1
#define DEBUG_STDOUT 2

#ifdef BUILD_DEBUG
#define DEBUG_OUTPUT DEBUG_NONE
#else
#define DEBUG_OUTPUT DEBUG_NONE
#endif


#define DVAL(what) #what"="<< (what)
#define BVAL(x) #x"="<<((x)?"true":"false")

#if DEBUG_OUTPUT == DEBUG_LOG

#define DBG(what) do { std::stringstream s; \
	s << what; \
	LOG(DEBUG) << s.str(); } while(0)\

#elif DEBUG_OUTPUT == DEBUG_STDOUT
#define DBG(what) std::cout<< what <<std::endl

#elif DEBUG_OUTPUT == DEBUG_NONE
#define DBG(what) do { } while(0)

#endif


#endif /* LOGGER_H_ */
