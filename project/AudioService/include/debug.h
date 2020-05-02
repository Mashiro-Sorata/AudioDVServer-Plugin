#pragma once

#ifndef DEBUG_H
#define DEBUG_H


//测试日志开关
//#define DEBUG_SWITCH

//转为宽字符
#define __T_(x) L ## x
#define __T(x) __T_(x)

#ifdef DEBUG_SWITCH
#include<iostream>

//用NERvLogInfo输出
#define NER_OUT

#ifdef NER_OUT

#define _T(x) __T(x)

#include <NERvGear/NERvSDK.h>
#include "../include/defs.h"

#define LOG_INFO(format, ...) NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), format)

#define LOG_DEBUG(format, ...) NERvGear::NERvLogDebug(NVG_TEXT(NAME_STRING), format)

#define LOG_WARN(format, ...) NERvGear::NERvLogWarn(NVG_TEXT(NAME_STRING), format)

#define LOG_ERROR(format, ...) NERvGear::NERvLogError(NVG_TEXT(NAME_STRING), format)

#else

#define _T(x) x

#define LOG(level, format, ...) \
{ \
	fprintf(stderr, "[%s](%s)>>>>%s: " format "\n", \
				 __func__, __FILE__, level, ##__VA_ARGS__ ); \
}

#define LOG_INFO(format, ...) LOG("INFO", format)
#define LOG_DEBUG(format, ...) LOG("DEBUG", format)
#define LOG_WARN(format, ...) LOG("WARN", format)
#define LOG_ERROR(format, ...) LOG("ERROR", format)

#endif
#else
#define _T(x) x

#define LOG_INFO(format, ...)

#define LOG_DEBUG(format, ...)

#define LOG_WARN(format, ...)

#define LOG_ERROR(format, ...)

#endif // TEST


#endif // !DEBUG_H
