/***************************************************************
 * Name:      adv_server.cpp
 * Purpose:   Code for Plug-in CAudioDVServer Class
 * Author:    Mashiro_Sorata (mashiro_sorata@qq.com)
 * Created:   2020-04-29
 * Copyright: Mashiro_Sorata (c)
 * License:   MIT
 **************************************************************/

 /// ============================================================================
 /// declarations
 /// ============================================================================

 /// ----------------------------------------------------------------------------
 /// headers
 /// ----------------------------------------------------------------------------
#include <NERvGear/NERvSDK.h>

#include "../include/main_adv_server.h"
#include "../include/defs.h"
#include "../include/a2fft_server.h"
#include "../include/config.h"
#include "../include/debug.h"

using namespace NERvGear;


/// ----------------------------------------------------------------------------
/// variables
/// ----------------------------------------------------------------------------
CA2FFTServer* audioServer;
char* srv_ip;
u_short srv_port = 5050;
int srv_maxconn = 5;

/// ----------------------------------------------------------------------------
/// plug-in declaration
/// ----------------------------------------------------------------------------

NVG_BEGIN_PLUGIN_INFO(CAudioDVServer)
NVG_DECLARE_PLUGIN_UID(0x16ED09AD, 0xE37E, 0x5653, 0xD2, 0xD9, 0xF1, 0x96, 0x93, 0xBB, 0x00, 0xBB)
NVG_DECLARE_PLUGIN_TYPE(PLUGIN::BASIC)
NVG_DECLARE_PLUGIN_VERSION(VER_MAJOR, VER_MINOR, VER_SUBMINOR)
NVG_DECLARE_PLUGIN_NAME(NAME_STRING)
NVG_DECLARE_PLUGIN_DESCRIP(DESCRIP_STRING)
NVG_DECLARE_PLUGIN_COMMENT(COMMENT_STRING)
NVG_DECLARE_PLUGIN_AUTHOR("Mashiro_Sorata")
NVG_DECLARE_PLUGIN_HOMEPAGE("mashirosorata.vicp.io")
NVG_DECLARE_PLUGIN_EMAIL("mashiro_sorata@qq.com")
NVG_END_PLUGIN_INFO(CAudioDVServer)

/// ----------------------------------------------------------------------------
/// component registration
/// ----------------------------------------------------------------------------

NVG_NO_COMPONENT_REGISTER(CAudioDVServer)

/// ============================================================================
/// implementation
/// ============================================================================

/// ----------------------------------------------------------------------------
/// event handlers
/// ----------------------------------------------------------------------------

long CAudioDVServer::OnInitial()
{
    LOG_INFO(L"Initial CAudioDVServer!");
    srv_ip = new char[17];
    ReadConfig(&srv_ip, &srv_port, &srv_maxconn);
    audioServer = new CA2FFTServer(srv_ip, srv_port, srv_maxconn);
    return PluginImpl::OnInitial();
}

long CAudioDVServer::OnRelease()
{
    LOG_INFO(L"Exit CAudioDVServer!");
    audioServer->ExitServer();
    delete[] srv_ip;
    delete audioServer;
    return PluginImpl::OnRelease();
}

long CAudioDVServer::OnReady()
{
    LOG_INFO(L"Start CAudioDVServer!");
    audioServer->StartServer();
    return PluginImpl::OnReady();
}

NVG_IMPLEMENT_PLUGIN(CAudioDVServer)