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



struct WebSocketStreamHeader {
	unsigned int header_size;				//数据包头大小
	int mask_offset;					//掩码偏移
	unsigned int payload_size;				//数据大小
	bool fin;                                               //帧标记
	bool masked;					        //掩码
	unsigned char opcode;					//操作码
	unsigned char res[3];
};

enum WS_Status
{
	WS_STATUS_CONNECT = 0,
	WS_STATUS_UNCONNECT = 1,
};


enum WS_FrameType
{
	WS_EMPTY_FRAME = 0xF0,
	WS_ERROR_FRAME = 0xF1,
	WS_TEXT_FRAME = 0x01,
	WS_BINARY_FRAME = 0x02,
	WS_PING_FRAME = 0x09,
	WS_PONG_FRAME = 0x0A,
	WS_OPENING_FRAME = 0xF3,
	WS_CLOSING_FRAME = 0x08
};



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
	static const u_short INTERVAL;
	//发送长度
	static const u_short SENDLENGTH;
	//单声道发送长度
	static const u_short MONOSENDLENGTH;
	
	static const int DataIndex[64];
	static const int Gap[64];

private:
	bool Initial();
	bool StartMainLoopService();
	bool StartBufferSenderService();
	static unsigned int __stdcall MainLoopService(PVOID pParam);
	static unsigned int __stdcall BufferSenderService(PVOID pParam);
	void SendToClients(char* buffer);
	
	static CADataCapture* audioCapture;

	//状态控制:1->运行,0->停止
	static std::atomic<bool> control;
	//记录连接到服务端的client数量
	static std::atomic<u_short> clientNum;

	//向量存放客户端地址
	static std::vector<SOCKET> clientsVector;
	//保证clientsVector不能同时操作
	static std::mutex clientsMutex;

	//每次处理的数据个数
	static const UINT32 dataSize;
	static const size_t complexSize;

	//Server的地址:默认0.0.0.0
	u_long ip_;
	//端口号:默认5050
	u_short port_;
	//最大连接数
	int maxClients_;

	//发送缓冲区
	static float* sendBuffer;
	static float* lSendBuffer;
	static float* rSendBuffer;

	//定义服务端套接字，接受请求套接字
	SOCKET socketServer_;
	//服务端地址客户端地址
	SOCKADDR_IN serverAddr_;

	HANDLE mainLoopServiceHandle_;
	unsigned int mainLoopServiceID_;
	HANDLE bufferSenderServiceHandle_;
	unsigned int bufferSenderServiceID_;
};

#endif // A2FFT_SERVER_H
