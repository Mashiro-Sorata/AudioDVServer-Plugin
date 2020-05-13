#include <Windows.h>
#include "../include/debug.h"



static void String2TCHAR(const std::string _str, TCHAR* tchar)
{
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)_str.c_str(), -1, tchar, 256);
}


void Debug_Var(char* name, long data)
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

void Debug_Var(char* name, char* data)
{
	std::string _str = name;
	_str.append(": ");
	_str.append(data);
	TCHAR _tchar[256];
	String2TCHAR(_str, _tchar);
	LOG_DEBUG(_tchar);
}

void Debug_Var(char* name, std::string str)
{
	std::string _str = name;
	_str.append(": ");
	_str.append(str);
	TCHAR _tchar[256];
	String2TCHAR(str, _tchar);
	LOG_DEBUG(_tchar);
}