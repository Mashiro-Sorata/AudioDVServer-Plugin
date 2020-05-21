#ifndef A2FFT_SERVER_H
#define A2FFT_SERVER_H

//include
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
#include<atomic>
#include<mutex>
#include<vector>
#include "../include/audiocapture.h"
//DEBUG
#include "../include/debug.h"



#define DEFAULT_IP_LOCAL "127.0.0.1"
#define DEFAULT_IP_ANY "0.0.0.0"
#define DEFAULT_PORT 5050
#define DEFAULT_MAXCLIENTS SOMAXCONN


//声明类
class CA2FFTServer
{
public:
	CA2FFTServer(const char* ip, u_short port, int maxClients);
	CA2FFTServer();
	~CA2FFTServer();

	bool StartServer();
	bool ExitServer();
	bool RebootServer();

	

	//检测客户端的间隔时间
	static const u_short sm_Interval;
	//发送长度
	static const u_short sm_SendLength;
	//单声道发送长度
	static const u_short sm_MonoSendLength;
	
	static const int sm_DataIndex[64];
	static const int sm_Gap[64];

private:
	bool Initial_();
	bool StartMainLoopService_();
	bool StartBufferSenderService_();
	static unsigned int __stdcall MainLoopService_(PVOID pParam);
	static unsigned int __stdcall BufferSenderService_(PVOID pParam);
	void SendToClients_(char* buffer);
	
	static CADataCapture* sm_pAudioCapture_;

	//状态控制:1->运行,0->停止
	static std::atomic<bool> sm_control_;
	//记录连接到服务端的client数量
	static std::atomic<u_short> sm_clientNum_;

	//向量存放客户端地址
	static std::vector<SOCKET> sm_clientsVector_;
	//保证clientsVector不能同时操作
	static std::mutex sm_clientsMutex_;

	//每次处理的数据个数
	static const UINT32 sm_DataSize_;
	static const size_t sm_ComplexSize_;

	//发送缓冲区
	static float* sm_pSendBuffer_;
	static float* sm_pLSendBuffer_;
	static float* sm_pRSendBuffer_;

	//Server的地址:默认0.0.0.0
	u_long m_ip_;
	//端口号:默认5050
	u_short m_port_;
	//最大连接数
	int m_maxClients_;

	//定义服务端套接字，接受请求套接字
	SOCKET m_socketServer_;
	//服务端地址客户端地址
	SOCKADDR_IN m_serverAddr_;

	HANDLE m_mainLoopServiceHandle_;
	unsigned int m_mainLoopServiceID_;
	HANDLE m_bufferSenderServiceHandle_;
	unsigned int m_bufferSenderServiceID_;
};

#endif // A2FFT_SERVER_H
