#include "debug.h"

static HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ?
		(HMODULE)mbi.AllocationBase : NULL);
}

static void TCHAR2Char(const TCHAR* tchar, char* _char)
{
	//获取字节长度   
	int iLength;
	iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
	//将tchar值赋给_char
	WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}

void String2TCHAR(const std::string _str, TCHAR* tchar)
{
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)_str.c_str(), -1, tchar, MAX_CHAR_LENGTH);
}

void GetInstanceFolderPath(std::string* dirPath)
{
	std::string exePath = "";
	TCHAR tcFullPath[MAX_PATH];
	char pChPath[MAX_PATH];
	memset(pChPath, '\0', MAX_PATH);
	/* 获取当前dll的执行路径exe路径 */
	GetModuleFileName(GetSelfModuleHandle(), tcFullPath, MAX_PATH);
	/** 将tchar转为char */
	TCHAR2Char(tcFullPath, pChPath);
	exePath = std::string(pChPath);
	size_t iPos = exePath.rfind("\\");
	*dirPath = exePath.substr(0, iPos + 1);
}



#ifdef DEBUG_SWITCH

#if LOGGER_TYPE==LOGGER_TYPE_FILE

const std::string debug::CLogger::sm_fileName_ = "ADV_Log.log";

#endif

const debug::LEVEL debug::CLogger::sm_maxLevel_ = debug::DebugLevel;

debug::CLogger::CLogger()
{
	m_flag_ = false;
#if LOGGER_TYPE==LOGGER_TYPE_FILE

	m_outfile_ = NULL;

#endif
}

debug::CLogger::~CLogger()
{
#if LOGGER_TYPE==LOGGER_TYPE_FILE

	if (NULL != m_outfile_)
	{
		m_outfile_->close();
		delete m_outfile_;
		m_outfile_ = NULL;
	}

#endif
}

void debug::CLogger::Initial(bool flag)
{
	m_flag_ = flag;
#if LOGGER_TYPE==LOGGER_TYPE_FILE
	if (m_flag_)
	{
		std::string _dirPath;
		GetInstanceFolderPath(&_dirPath);
		_dirPath += sm_fileName_;
		std::ofstream _temp(_dirPath, std::ios::out);
		_temp.close();
		m_outfile_ = new std::ofstream(_dirPath, std::ios::app);
	}
#endif
}

void debug::CLogger::Log_Info(const char* _FILE, const char* _func, const char* format)
{
	if (sm_maxLevel_ >= debug::LEVEL::_INFO_)
		Log_Base(_FILE, _func, debug::LEVEL::_INFO_, "INFO", format);
}

void debug::CLogger::Log_Debug(const char* _FILE, const char* _func, const char* format)
{
	if (sm_maxLevel_ >= debug::LEVEL::_DEBUG_)
		Log_Base(_FILE, _func, debug::LEVEL::_DEBUG_, "DEBUG", format);
}

void debug::CLogger::Log_Warn(const char* _FILE, const char* _func, const char* format)
{
	if (sm_maxLevel_ >= debug::LEVEL::_WARN_)
		Log_Base(_FILE, _func, debug::LEVEL::_WARN_, "WARN", format);
}

void debug::CLogger::Log_Error(const char* _FILE, const char* _func, const char* format)
{
	if (sm_maxLevel_ >= debug::LEVEL::_ERROR_)
		Log_Base(_FILE, _func, debug::LEVEL::_ERROR_, "ERROR", format);
}

#if LOGGER_TYPE==LOGGER_TYPE_NEROUT

void debug::CLogger::NerLog_(LEVEL level, std::string headLog, const char* sdata)
{
	TCHAR _tchar[MAX_CHAR_LENGTH];
	String2TCHAR(headLog, _tchar);
	switch (level)
	{
	case(LEVEL::_INFO_):
		NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), _tchar);
		break;
	case(LEVEL::_DEBUG_):
		NERvGear::NERvLogDebug(NVG_TEXT(NAME_STRING), _tchar);
		break;
	case(LEVEL::_WARN_):
		NERvGear::NERvLogWarn(NVG_TEXT(NAME_STRING), _tchar);
		break;
	case(LEVEL::_ERROR_):
		NERvGear::NERvLogError(NVG_TEXT(NAME_STRING), _tchar);
		break;
	default:
		NERvGear::NERvLogDebug(NVG_TEXT(NAME_STRING), _tchar);
	}
}

void debug::CLogger::Log2Ner_(LEVEL level, std::string headLog, long data)
{
	char sdata[22];
	_ltoa_s(data, sdata, 10);
	headLog.append(sdata);
	NerLog_(level, headLog, sdata);
}

void debug::CLogger::Log2Ner_(LEVEL level, std::string headLog, char* data)
{
	headLog.append(data);
	NerLog_(level, headLog, data);
}

void debug::CLogger::Log2Ner_(LEVEL level, std::string headLog, const char* data)
{
	headLog.append(data);
	NerLog_(level, headLog, data);
}

void debug::CLogger::Log2Ner_(LEVEL level, std::string headLog, std::string data)
{
	headLog.append(data);
	NerLog_(level, headLog, data.c_str());
}

#endif

debug::CLogger debug::LOGGER = debug::CLogger();



#endif // DEBUG_SWITCH