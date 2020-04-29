#pragma once

#ifndef DEBUG_H
#define DEBUG_H

//≤‚ ‘
#define DEBUG_SWITCH
//”√NERvLogInfo ‰≥ˆ
#define NER_OUT


#ifdef DEBUG_SWITCH

#include<iostream>
#define OFF   (0)
#define FATAL (1)
#define ERR (2)
#define WARN  (3)
#define INFO  (4)
#define DEBUG (5)
#define TRACE (6)

#define LOG_LEVEL DEBUG

#ifdef NER_OUT

#include <NERvGear/NERvSDK.h>
#include "../include/defs.h"

#define LOG(level, format, ...) \
	if(LOG_LEVEL >= level) { \
		NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), format); \
	}

#else
#define LOG(level, format, ...) \
	if(LOG_LEVEL >= level) { \
		fprintf(stderr, format "\n", \
				#level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    }
#endif
#else
#define LOG(level, format, ...)
#endif // TEST


#endif // !DEBUG_H
