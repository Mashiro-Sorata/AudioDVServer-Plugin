#include "../include/config.h"
#include "../include/a2fft_server.h"
#include "../include/debug.h"


void ReadConfig(char** _ip, unsigned short* _port, int* _maxClient)
{
    LOG_INFO(_T("¶ÁÈ¡ÅäÖÃÎÄ¼ş..."));
    std::string dirPath;
    GetInstanceFolderPath(&dirPath);
    //std::ofstream out("log.txt");
    //out << dirPath << std::endl;
    //out.close();
    TCHAR wdirPath[MAX_PATH];
    TCHAR w_ip[17];
    char c_ip[17];
    String2TCHAR(dirPath + CONFIGFILE, wdirPath);
    GetPrivateProfileString(NVG_TEXT("server"), NVG_TEXT("ip"), NVG_TEXT(DEFAULT_IP_LOCAL), w_ip, 17, wdirPath);
    if (!lstrcmp(NVG_TEXT("ANY"),w_ip))
    {
        LOG_DEBUG(_T("INADDR_ANY"));
        strcpy(*_ip, DEFAULT_IP_ANY);
    }
    else
    {
        LOG_DEBUG(_T("INADDR_LOCAL"));
        strcpy(*_ip, DEFAULT_IP_LOCAL);
    }
    *_port = GetPrivateProfileInt(NVG_TEXT("server"), NVG_TEXT("port"), DEFAULT_PORT, wdirPath);
    *_maxClient = GetPrivateProfileInt(NVG_TEXT("server"), NVG_TEXT("maxclient"), DEFAULT_MAXCLIENTS, wdirPath);
}

