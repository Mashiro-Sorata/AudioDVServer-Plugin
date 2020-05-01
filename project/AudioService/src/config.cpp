#include <fstream>
#include "../include/config.h"



bool ReadConfig(char** _ip, unsigned short* _port, int* _maxconn)
{
    Json::Value root;
    std::ifstream ifs(CONFIGFILE);
    if (ifs.fail())
    {
        LOG_WARN(L"打开文件失败!");
        *_ip = NULL;
        *_port = 0;
        *_maxconn = 0;
        return false;
    }
    LOG_WARN(L"打开文件成功!");
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    if (!parseFromStream(builder, ifs, &root, NULL))
    {
        *_ip = NULL;
        *_port = 0;
        *_maxconn = 0;
        return false;
    }
    try
    {
        std::string sip = root["ip"].asString();
        memcpy(*_ip, sip.c_str(), sip.size() + 1);
        *_port = root["port"].asUInt();
        *_maxconn = root["maxconn"].asInt();
    }
    catch (Json::LogicError e)
    {
        *_ip = NULL;
        *_port = 0;
        *_maxconn = 0;
        return false;
    }
    return true;
}

bool IsIpFormatRight(char* ipAddress)
{//判断IP地址是否合法
    if (NULL == ipAddress)
    {
        return false;
    }
    int a, b, c, d;
    if ((sscanf_s(ipAddress, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
        && (a >= 0 && a <= 255)
        && (b >= 0 && b <= 255)
        && (c >= 0 && c <= 255)
        && (d >= 0 && d <= 255))
    {
        return true;
    }
    return false;
}