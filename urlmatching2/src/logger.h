/*
 * logger.h
 *
 *  Created on: 9 במאי 2015
 *      Author: Daniel
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include "easylogging++.h"

#define STR(s) #s
#define XSTR(a) STR(a)

#define DEBUG_LOG 1
//#define DEBUG_STDOUT 1
//#define DEBUG_NONE 1

#define DVAL(what) #what"="<< (what)
#define BVAL(x) #x"="<<((x)?"true":"false")

#if DEBUG_LOG > 0

#define DBG(what) do { std::stringstream s; \
	s << what; \
	LOG(DEBUG) << s.str(); } while(0)\


#elif DEBUG_STDOUT > 0
#define DBG(what) std::cout<< what <<std::endl

#elif DEBUG_NONE > 0
#define DBG(x) do { } while(0)

#endif


#endif /* LOGGER_H_ */
