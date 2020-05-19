#ifndef DEBUG_H
#define DEBUG_H

#include <NERvGear/NERvSDK.h>
#include <windows.h>
#include <iostream>

#define MAX_CHAR_LENGTH 256
#define _T(x) x

void String2TCHAR(const std::string _str, TCHAR* tchar);
void GetInstanceFolderPath(std::string* dirPath);

//调试开关
#define DEBUG_SWITCH

#ifdef DEBUG_SWITCH

#include <fstream>
#include "../include/defs.h"

namespace debug
{
	enum LOGTYPE { T_FILE, T_CONSOLE, T_NEROUT };
	enum LEVEL { _INFO_ = 0, _DEBUG_ = 1, _WARN_ = 2, _ERROR_ = 3, _ALL_ = 4 };

	//控制输出等级
	const debug::LEVEL DEBUGLEVEL(LEVEL::_ALL_);

	// LOGTYPE定义Logger的类型
	const debug::LOGTYPE LOGGERTYPE(LOGTYPE::T_FILE);

	class Logger
	{
	public:
		Logger(LOGTYPE type);
		~Logger();
		void Initial();
		void Log_Info(const char* _FILE, const char* _func, const char* format);
		void Log_Debug(const char* _FILE, const char* _func, const char* format);
		void Log_Warn(const char* _FILE, const char* _func, const char* format);
		void Log_Error(const char* _FILE, const char* _func, const char* format);
		template<typename T> void Log_Base(const char* _FILE, const char* _func, LEVEL level, const char* name, T format);

	private:
		void Log2Ner(LEVEL level, std::string headLog, long data);
		void Log2Ner(LEVEL level, std::string headLog, char* data);
		void Log2Ner(LEVEL level, std::string headLog, const char* data);
		void Log2Ner(LEVEL level, std::string headLog, std::string data);
		void NerLog(LEVEL level, std::string headLog, const char* sdata);

		static const std::string FILENAME;
		static const LEVEL maxLevel;
		LOGTYPE type_;
		std::ofstream* outfile_;
		SYSTEMTIME now_;
	};

	template<typename T>
	void Logger::Log_Base(const char* _FILE, const char* _func, LEVEL level, const char* name, T format)
	{
		GetLocalTime(&now_);
		char sdata[5];
		std::string _headLog;
		_headLog.append("<");
		_itoa_s(now_.wHour, sdata, 10);
		_headLog.append(sdata);
		_headLog.append(":");
		_itoa_s(now_.wMinute, sdata, 10);
		_headLog.append(sdata);
		_headLog.append(":");
		_itoa_s(now_.wSecond, sdata, 10);
		_headLog.append(sdata);
		_headLog.append(">");
		_headLog.append("[");
		_headLog.append(_FILE);
		_headLog.append("](Function: ");
		_headLog.append(_func);
		_headLog.append(")>>>>");
		_headLog.append(name);
		_headLog.append(": ");
		switch (type_)
		{
		case(LOGTYPE::T_NEROUT):
			Log2Ner(level, _headLog, format);
			break;
		case(LOGTYPE::T_FILE):
			(*outfile_) << _headLog << format << std::endl;
			break;
		case(LOGTYPE::T_CONSOLE):
			std::cout << _headLog << format << std::endl;
			break;
		}

	}

	extern Logger LOGGER;
}

#define LOG_INIT() debug::LOGGER.Initial()
#define LOG_INFO(format) debug::LOGGER.Log_Info(__FILE__, __func__, format)
#define LOG_DEBUG(format) debug::LOGGER.Log_Debug(__FILE__, __func__, format)
#define LOG_WARN(format) debug::LOGGER.Log_Warn(__FILE__, __func__, format)
#define LOG_ERROR(format) debug::LOGGER.Log_Error(__FILE__, __func__, format)
#define LOG_BASE(name, format) debug::LOGGER.Log_Base(__FILE__, __func__, debug::LEVEL::_ALL_, name, format);

#else

#define LOG_INIT()
#define LOG_INFO(format, ...)
#define LOG_DEBUG(format, ...)
#define LOG_WARN(format, ...)
#define LOG_ERROR(format, ...)
#define LOG_BASE(name, format)

#endif // !DEBUG_SWITCH
#endif // !DEBUG_H

