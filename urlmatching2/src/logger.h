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

#define DEBUG_NONE 0
#define DEBUG_LOG 1
#define DEBUG_STDOUT 2

#define DEBUG_OUTPUT DEBUG_LOG

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
