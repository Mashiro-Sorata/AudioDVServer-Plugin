#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include "json/json.h"
#include "debug.h"


#define CONFIGFILE "./Plugins/CA2FFTServer/config.json"

bool ReadConfig(char** _ip, unsigned short* _port, int* _maxconn);
bool IsIpFormatRight(char* ipAddress);


#endif // !CONFIG_H
