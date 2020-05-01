#pragma once
#ifndef CONFIG_H
#define CONFIG_H


#define CONFIGFILE "advConfig.ini"

void ReadConfig(char** _ip, unsigned short* _port, int* _maxconn);


#endif // !CONFIG_H
