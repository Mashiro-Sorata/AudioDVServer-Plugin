#pragma once

#ifndef A2FFT_SERVER_H
#define A2FFT_SERVER_H

//include
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
#include<atomic>
#include<mutex>
#include<vector>
#include "../include/AudioFFT.h"
#include "../include/audiocapture.h"

//DEBUG
#include "../include/debug.h"

class CA2FFTServer
{
public:
	CA2FFTServer(const char* ip, u_short port, int maxconn);
	CA2FFTServer();
	~CA2FFTServer();

	virtual bool Initial();
	virtual inline bool StartServer();
	virtual inline bool ExitServer();
	virtual inline bool RebootServer();

	

	//检测客户端的间隔时间
	static const u_short INTERVAL;
	//默认端口号
	static const u_short PORT;
	//发送长度
	static const u_short SENDLENGTH;
	//单声道发送长度
	static const u_short MONOSENDLENGTH;
	
	static const int DataIndex[64];
	static const int Gap[64];

private:

	static unsigned int __stdcall MainLoopService(PVOID pParam);
	static unsigned int __stdcall BufferSenderService(PVOID pParam);
	bool StartMainLoopService();
	bool StartBufferSenderService();

	void SendToClients(char* buffer);
	
	static CADataCapture* audioCapture;

	//状态控制:1->运行,0->停止
	static std::atomic<bool> control;
	//连接到服务端的client数量,减少对子线程的阻塞
	static std::atomic<u_short> clientNum;

	//向量存放客户端地址
	static std::vector<SOCKET> clientsVector;
	//保证clients不能同时操作
	static std::mutex clientsMutex;

	//将buff区以float的形式读取
	//static float* pfData;
	//数据包定位符，将数据包的数据凑满dataSize个后进行处理
	//static UINT32 desPtn;
	//static UINT32 srcPtn;
	//static INT32 packRem;

	//FFT
	//static audiofft::AudioFFT fft;
	//每次处理的数据个数
	static const UINT32 dataSize;
	static const size_t complexSize;

	//static std::vector<float> lData;
	//static std::vector<float> rData;

	//static std::vector<float> lRe;
	//static std::vector<float> lImg;
	//static std::vector<float> rRe;
	//static std::vector<float> rImg;

	//Server的地址:默认0.0.0.0
	u_long ip_;
	//端口号:默认5050
	u_short port_;
	//最大连接数
	int maxConN_;

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
