#pragma once

#ifndef DEBUG_H
#define DEBUG_H

//测试日志开关
#define DEBUG_SWITCH


#ifdef DEBUG_SWITCH
#include<iostream>

//用NERvLogInfo输出
#define NER_OUT
#ifdef NER_OUT

#include <NERvGear/NERvSDK.h>
#include "../include/defs.h"

#define LOG_INFO(format, ...) NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), format)

#define LOG_DEBUG(format, ...) NERvGear::NERvLogDebug(NVG_TEXT(NAME_STRING), format)

#define LOG_WARN(format, ...) NERvGear::NERvLogWarn(NVG_TEXT(NAME_STRING), format)

#define LOG_ERROR(format, ...) NERvGear::NERvLogError(NVG_TEXT(NAME_STRING), format)

#else
#include <locale.h>

#define LOG_INFO(format, ...) \
{ \
	setlocale(LC_CTYPE, ".936");\
	wprintf(L"%ls: %ls\n", L"INFO", format);\
}

#define LOG_DEBUG(format, ...) \
{ \
	setlocale(LC_CTYPE, ".936");\
	wprintf(L"%ls: %ls\n", L"DEBUG", format);\
}

#define LOG_WARN(format, ...) \
{ \
	setlocale(LC_CTYPE, ".936");\
	wprintf(L"%ls: %ls\n", L"WARN", format);\
}

#define LOG_ERROR(format, ...) \
{ \
	setlocale(LC_CTYPE, ".936");\
	wprintf(L"%ls: %ls\n", L"ERROR", format);\
}

#endif
#else

#define LOG_INFO(format, ...)

#define LOG_DEBUG(format, ...)

#define LOG_WARN(format, ...)

#define LOG_ERROR(format, ...)

#endif // TEST


#endif // !DEBUG_H
