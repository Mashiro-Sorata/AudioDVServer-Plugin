/***************************************************************
 * Name:      adv_server.h
 * Purpose:   Defines Plug-in CAudioDVServer Class
 * Author:    Mashiro_Sorata (mashiro_sorata@qq.com)
 * Created:   2020-04-29
 * Copyright: Mashiro_Sorata (c)
 * License:   MIT
 **************************************************************/

#ifndef ADV_SERVER_H
#define ADV_SERVER_H

#include <NERvGear/plugin.h>

 /// ----------------------------------------------------------------------------
 /// \class CAudioDVServer
 /// \brief CAudioDVServer Plug-in
 /// ----------------------------------------------------------------------------

class CAudioDVServer :public NERvGear::PluginImpl
{
    virtual long NVG_METHOD OnInitial();
    virtual long NVG_METHOD OnReady();
    virtual long NVG_METHOD OnRelease();

    NVG_DECLARE_PLUGIN(CAudioDVServer)
};

#endif // !ADV_SERVER_H
