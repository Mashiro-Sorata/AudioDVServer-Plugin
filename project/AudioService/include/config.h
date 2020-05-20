#pragma once
#ifndef CONFIG_H
#define CONFIG_H


#define CONFIGFILE "advConfig.ini"

void ReadConfig(char** _ip, unsigned short* _port, int* _maxClient, bool* _logger);


#endif // !CONFIG_H
