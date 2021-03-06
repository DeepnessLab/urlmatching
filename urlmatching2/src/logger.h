/*
 * logger.h
 *
 *  Created on: 18 December 2014
 *      Author: Daniel Krauthgamer
 *
 *  Logging header configuration.
 *  Define where DEBUG_OUTPUT is streamed.
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <stdio.h>
#include "easylogging++.h"



//Debug output options:
//---------------------
#define DBG_TO_NONE 0
#define DBG_TO_LOG 1
#define DBG_TO_STDOUT 2

//Debug output configuration:
//--------------------------
#ifdef BUILD_DEBUG
#define DEBUG_OUTPUT DBG_TO_NONE
#else
#define DEBUG_OUTPUT DBG_TO_NONE
#endif

#define IS_EASYLOGGING_DEFINED (DEBUG_OUTPUT == DBG_TO_LOG)


#ifdef BUILD_DEBUG
#define ON_DEBUG_ONLY(what) do {what ; } while (0)
#else
#define ON_DEBUG_ONLY(what) do {} while (0)
#endif



#ifdef BUILD_DEBUG
	#if DEBUG_OUTPUT == DBG_TO_LOG
	#define ASSERT(what) if (!(what)) { \
		el::Loggers::flushAll();\
		assert(what); }
	#else
	#define ASSERT(what) assert(what)
	#endif
#else
#define ASSERT(what) do {} while(0)
#endif

#define STR(s) #s
#define XSTR(a) STR(a)


#define DVAL(what) #what"="<< (what)
#define BVAL(x) #x"="<<((x)?"true":"false")

#if DEBUG_OUTPUT == DBG_TO_LOG
#define DBG(what) do { std::stringstream s; \
	s << what; \
	LOG(DEBUG) << s.str(); } while(0)
#elif DEBUG_OUTPUT == DBG_TO_STDOUT
#define DBG(what) std::cout<< what <<std::endl
#elif DEBUG_OUTPUT == DBG_TO_NONE
#define DBG(what) do { } while(0)
#endif

#define COND_DBG(flag,what) \
	if (flag)  {\
		DBG(what);\
	}

#endif /* LOGGER_H_ */
