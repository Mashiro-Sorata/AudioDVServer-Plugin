#include "../include/config.h"
#include "../include/a2fft_server.h"
#include "../include/debug.h"

#define __T(x) L ## x
#define _T(x) __T(x)

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
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)_str.c_str(), -1, tchar, 256);
}

static void GetInstanceFolderPath(std::string* dirPath)
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
    *dirPath = exePath.substr(0, iPos+1);
}


void ReadConfig(char** _ip, unsigned short* _port, int* _maxClient)
{
    LOG_INFO(_T("读取配置文件..."));
    std::string dirPath;
    GetInstanceFolderPath(&dirPath);
    //std::ofstream out("log.txt");
    //out << dirPath << std::endl;
    //out.close();
    TCHAR wdirPath[MAX_PATH];
    TCHAR w_ip[17];
    char c_ip[17];
    String2TCHAR(dirPath + CONFIGFILE, wdirPath);
    GetPrivateProfileString(_T("server"), _T("ip"), _T(DEFAULT_IP_LOCAL), w_ip, 17, wdirPath);
    if (!lstrcmp(_T("ANY"),w_ip))
    {
        LOG_DEBUG(_T("INADDR_ANY"));
        strcpy(*_ip, DEFAULT_IP_ANY);
    }
    else
    {
        LOG_DEBUG(_T("INADDR_ANY"));
        strcpy(*_ip, DEFAULT_IP_LOCAL);
    }
    *_port = GetPrivateProfileInt(_T("server"), _T("port"), DEFAULT_PORT, wdirPath);
    *_maxClient = GetPrivateProfileInt(_T("server"), _T("maxclient"), DEFAULT_MAXCLIENTS, wdirPath);
}

