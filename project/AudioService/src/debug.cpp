#include <Windows.h>
#include "debug.h"


#ifdef DEBUG_SWITCH

#ifdef NER_OUT

static void String2TCHAR(const std::string _str, TCHAR* tchar)
{
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)_str.c_str(), -1, tchar, 256);
}


void Debug_Var(const char* name, long data)
{
	char sdata[22];
	_itoa_s(data, sdata, 10);
	std::string _str = name;
	_str.append(": ");
	_str.append(sdata);
	TCHAR _tchar[256];
	String2TCHAR(_str, _tchar);
	LOG_DEBUG(_tchar);
}

void Debug_Var(const char* name, char* data)
{
	std::string _str = name;
	_str.append(": ");
	_str.append(data);
	TCHAR _tchar[256];
	String2TCHAR(_str, _tchar);
	LOG_DEBUG(_tchar);
}

void Debug_Var(const char* name, std::string data)
{
	std::string _str = name;
	_str.append(": ");
	_str.append(data);
	TCHAR _tchar[256];
	String2TCHAR(data, _tchar);
	LOG_DEBUG(_tchar);
}

#else

void Debug_Var(const char* name, long data)
{
	std::cout << name << ": " << data << std::endl;
}

void Debug_Var(const char* name, char* data)
{
	std::cout << name << ": " << data << std::endl;
}

void Debug_Var(const char* name, std::string data)
{
	std::cout << name << ": " << data << std::endl;
}


#endif // NER_OUT

#endif // DEBUG_SWITCH
