#include <cmath>
#include "../include/a2fft_server.h"


//与SENDLENGTH相关
//将complexSize个fft数据压缩为SENDLENGTH/2个
const int CA2FFTServer::DataIndex[64] = { 1, 3, 5, 6, 8, 9, 10, 11, 13, 14, 15, 17, 18, 20, 22, 24, 27, 29, 32, 35, 38, 42, 46, 50, 55, 60, 66, 72, 79, 87, 95, 104, 114, 125, 137, 149, 164, 179, 196, 215, 235, 257, 281, 308, 337, 369, 404, 442, 484, 529, 579, 634, 694, 759, 831, 909, 995, 1089, 1192, 1304, 1427, 1562, 1710, 2048 };
const int CA2FFTServer::Gap[64] = { 1, 2, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 2, 3, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 12, 15, 15, 17, 19, 20, 22, 24, 27, 29, 32, 35, 38, 42, 45, 50, 55, 60, 65, 72, 78, 86, 94, 103, 112, 123, 135, 148, 338 };



const u_short CA2FFTServer::PORT = 5050;
const u_short CA2FFTServer::INTERVAL = 5;
const u_short CA2FFTServer::SENDLENGTH = 128;
const u_short CA2FFTServer::MONOSENDLENGTH = CA2FFTServer::SENDLENGTH / 2;

std::atomic<bool> CA2FFTServer::control = false;
std::vector<SOCKET> CA2FFTServer::clientsVector;
std::mutex CA2FFTServer::clientsMutex;
std::atomic<u_short> CA2FFTServer::clientNum = 0;

CADataCapture* CA2FFTServer::audioCapture;
const UINT32 CA2FFTServer::dataSize = 4096;
const size_t CA2FFTServer::complexSize = audiofft::AudioFFT::ComplexSize(dataSize);

float* CA2FFTServer::sendBuffer = NULL;
float* CA2FFTServer::lSendBuffer = NULL;
float* CA2FFTServer::rSendBuffer = NULL;


CA2FFTServer::CA2FFTServer(const char* ip, u_short port, int maxconn)
{
	ip_ = inet_addr(ip);
	port_ = (port < 1 || port > 65535) ? htons(PORT) : htons(port);
	maxConN_ = (maxconn < 1) ? SOMAXCONN : maxconn;
	socketServer_ = NULL;
	mainLoopServiceID_ = NULL;
	sendBuffer = new float[SENDLENGTH];
	lSendBuffer = new float[MONOSENDLENGTH];
	rSendBuffer = new float[MONOSENDLENGTH];
	clientsVector.reserve(maxConN_);

	audioCapture = new CADataCapture();
}

CA2FFTServer::CA2FFTServer()
{
	ip_ = htonl(INADDR_LOOPBACK);
	port_ = htons(PORT);
	maxConN_ = SOMAXCONN;
	socketServer_ = NULL;
	mainLoopServiceID_ = NULL;
	sendBuffer = new float[SENDLENGTH];
	lSendBuffer = new float[MONOSENDLENGTH];
	rSendBuffer = new float[MONOSENDLENGTH];
	clientsVector.reserve(maxConN_);

	audioCapture = new CADataCapture();
}

CA2FFTServer::~CA2FFTServer()
{
	audioCapture->Stop();
	clientsMutex.lock();
	CloseHandle(mainLoopServiceHandle_);
	CloseHandle(bufferSenderServiceHandle_);
	clientsMutex.unlock();
	//关闭套接字
	closesocket(socketServer_);
	
	delete[] sendBuffer;
	delete[] lSendBuffer;
	delete[] rSendBuffer;
	delete audioCapture;
	audioCapture = NULL;
	sendBuffer = NULL;
	lSendBuffer = NULL;
	rSendBuffer = NULL;
	//释放DLL资源
	WSACleanup();
}

bool CA2FFTServer::Initial()
{
	//版本号
	WORD w_req = MAKEWORD(2, 2);
	WSADATA wsadata;

	//初始化Windows Sockets DLL
	if (WSAStartup(w_req, &wsadata) != 0) {
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"初始化套接字库失败!");
		LOG(ERR, L"初始化套接字库失败!");
		return false;
	}
	else
	{
		LOG(INFO, L"初始化套接字库成功!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"初始化套接字库成功!");
	}
	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		LOG(ERR, L"套接字库版本号不符!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"套接字库版本号不符!");
		WSACleanup();
		return false;
	}
	else
	{
		LOG(INFO, L"套接字库版本正确!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"套接字库版本正确!");
	}
	//填充服务端信息
	serverAddr_.sin_family = AF_INET;
	serverAddr_.sin_addr.S_un.S_addr = ip_;
	serverAddr_.sin_port = port_;
	//创建套接字
	socketServer_ = socket(AF_INET, SOCK_STREAM, 0);
	//接受连接请求

	BOOL bReUseAddr = TRUE;
	setsockopt(socketServer_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReUseAddr, sizeof(BOOL));
	BOOL  bDontLinger = FALSE;
	setsockopt(socketServer_, SOL_SOCKET, SO_DONTLINGER, (const char*)&bDontLinger, sizeof(BOOL));
	if (bind(socketServer_, (SOCKADDR*)&serverAddr_, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		LOG(ERR, L"套接字绑定失败!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"套接字绑定失败!");
		WSACleanup();
		return false;
	}
	else
	{
		LOG(INFO, L"套接字绑定成功!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"套接字绑定成功!");
	}

	//设置套接字为监听状态
	if (listen(socketServer_, SOMAXCONN) < 0)
	{
		LOG(ERR, L"设置监听状态失败!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"设置监听状态失败!");
		WSACleanup();
		return false;
	}
	else
	{
		LOG(INFO, L"设置监听状态成功!");
		//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"设置监听状态成功!");
	}

	if (FAILED(audioCapture->Initial()))
	{
		return false;;
	}
	return true;
}

unsigned int __stdcall CA2FFTServer::MainLoopService(PVOID pParam)
{
	int len = sizeof(SOCKADDR);
	SOCKADDR_IN acceptAddr;
	SOCKET socketAccept;
	while (control)
	{
		if (clientNum < ((CA2FFTServer*)pParam)->maxConN_)
		{
			socketAccept = accept(((CA2FFTServer*)pParam)->socketServer_,
				(SOCKADDR*)&acceptAddr, &len);
			//LOG(INFO, "进入MainLoopService");
			if (socketAccept == SOCKET_ERROR)
			{
				LOG(WARN, L"连接失败!");
				//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"连接失败!");
			}
			else
			{
				if (clientNum == 0)
				{
					audioCapture->Start();
				}
				//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"连接成功!");
				clientNum++;
				clientsMutex.lock();
				clientsVector.push_back(socketAccept);
				clientsMutex.unlock();
				LOG(INFO, L"连接成功!");
			}
		}
	}
	return 0;
}

bool CA2FFTServer::StartMainLoopService()
{
	mainLoopServiceHandle_ = (HANDLE)_beginthreadex(NULL, 0, MainLoopService, this, 0, &mainLoopServiceID_);
	return (NULL != mainLoopServiceHandle_) ? true : false;
}

void CA2FFTServer::SendToClients(char* buffer)
{
	int send_len = 0;
	clientsMutex.lock();
	std::vector<SOCKET>::iterator itr = clientsVector.begin();
	while (itr != clientsVector.end())
	{
		send_len = 0;
		send_len = send(*itr, buffer, SENDLENGTH * sizeof(float), 0);
		if (send_len < 0)
		{
			//客户端断开连接
			LOG(WARN, L"发送失败!");
			//NERvGear::NERvLogInfo(NVG_TEXT(NAME_STRING), L"发送失败!");
			if (clientNum == 1)
			{
				audioCapture->Stop();
			}
			clientNum--;
			closesocket(*itr);
			itr = clientsVector.erase(itr);
		}
		else
		{
			itr++;
		}
	}
	clientsMutex.unlock();
}

unsigned int __stdcall CA2FFTServer::BufferSenderService(PVOID pParam)
{
	float* pfData;
	UINT32 desPtn = 0;
	UINT32 srcPtn = 0;
	INT32 packRem = 0;
	audiofft::AudioFFT fft;
	fft.init(dataSize);
	std::vector<float> lData;
	std::vector<float> rData;
	std::vector<float> lRe;
	std::vector<float> lImg;
	std::vector<float> rRe;
	std::vector<float> rImg;
	lData.reserve(dataSize);
	rData.reserve(dataSize);
	lRe.resize(complexSize);
	lImg.resize(complexSize);
	rRe.resize(complexSize);
	rImg.resize(complexSize);

	//fft幅值
	std::vector<float> lModel;
	std::vector<float> rModel;
	lModel.reserve(complexSize);
	rModel.reserve(complexSize);
	//临时变量
	unsigned int j = 0;
	float lSum = 0.0;
	float rSum = 0.0;
	
	//循环更新数据后发送的操作
	HRESULT hr;
	while (control)
	{
		//当有客户端连接时采集音频数据处理
		if (clientNum > 0)
		{
			//TO DO: 更新音频数据，得到FFT数据复制到s
			//发送缓冲区数据给所有的client
			hr = audioCapture->get_NextPacketSize();
			if (audioCapture->packetLength == 0)
			{
				continue;
			}
			hr = audioCapture->get_Buffer();
			packRem = desPtn + (audioCapture->numFramesAvailable) - dataSize;
			float* pfData = (float*)(audioCapture->pData);
			if (packRem < 0)
			{
				for (unsigned int i = 0; i < (audioCapture->numFramesAvailable * 2); i += 2)
				{
					lData.push_back(*(pfData + i));
					rData.push_back(*(pfData + i + 1));
				}
				desPtn += audioCapture->numFramesAvailable;
				srcPtn = 0;
			}
			else if (packRem > 0)
			{
				while (TRUE)
				{
					for (unsigned int i = 0; i < (dataSize - desPtn) * 2; i += 2)
					{
						lData.push_back(*(pfData + srcPtn + i));
						rData.push_back(*(pfData + srcPtn + i + 1));
					}
					srcPtn += dataSize - desPtn;
					desPtn = 0;
					fft.fft(&lData[0], &lRe[0], &lImg[0]);
					fft.fft(&rData[0], &rRe[0], &rImg[0]);
					//数据压缩处理，非线性段求均值，单声道压缩至64个数据
					j = 0;
					for (unsigned int i = 0; i < MONOSENDLENGTH; i++)
					{
						lSum = 0.0;
						rSum = 0.0;
						while (j < DataIndex[i])
						{
							if (j > complexSize - 1) 
							{
								control = false;
								return 1;
							}
							//取模
							lModel.push_back(sqrt(lRe[j] * lRe[j] + lImg[j] * lImg[j]));
							rModel.push_back(sqrt(rRe[j] * rRe[j] + rImg[j] * rImg[j]));
							lSum += lModel.back();
							rSum += rModel.back();
							j++;
						}
						lSendBuffer[i] = lSum / Gap[i];
						rSendBuffer[i] = rSum / Gap[i];
					}
					//将压缩的数据拼凑至sendBuffer_
					memcpy(sendBuffer, lSendBuffer, sizeof(float) * MONOSENDLENGTH);
					memcpy((sendBuffer + MONOSENDLENGTH), rSendBuffer, sizeof(float) * MONOSENDLENGTH);
					((CA2FFTServer*)pParam)->SendToClients((char*)sendBuffer);
					lData.clear();
					rData.clear();
					lModel.clear();
					rModel.clear();
					if (dataSize < packRem)
					{
						packRem -= dataSize;
					}
					else
					{
						break;
					}
				}
				for (unsigned int i = 0; i < packRem * 2; i += 2)
				{
					lData.push_back(*(pfData + i));
					rData.push_back(*(pfData + i + 1));
				}
				desPtn += packRem;
				srcPtn = 0;
			}
			else
			{
				for (unsigned int i = 0; i < audioCapture->numFramesAvailable * 2; i += 2)
				{
					lData.push_back(*(pfData + i));
					rData.push_back(*(pfData + i + 1));
				}
				desPtn = 0;
				srcPtn = 0;
				fft.fft(&lData[0], &lRe[0], &lImg[0]);
				fft.fft(&rData[0], &rRe[0], &rImg[0]);
				//数据压缩处理，非线性段求均值，单声道压缩至64个数据
				j = 0;
				for (unsigned int i = 0; i < MONOSENDLENGTH; i++)
				{
					lSum = 0.0;
					rSum = 0.0;
					while (j < DataIndex[i])
					{
						if (j > complexSize - 1)
						{
							control = false;
							return 1;
						}
						//取模
						lModel.push_back(sqrt(lRe[j] * lRe[j] + lImg[j] * lImg[j]));
						rModel.push_back(sqrt(rRe[j] * rRe[j] + rImg[j] * rImg[j]));
						lSum += lModel.back();
						rSum += rModel.back();
						j++;
					}
					lSendBuffer[i] = lSum / Gap[i];
					rSendBuffer[i] = rSum / Gap[i];
				}
				//将压缩的数据拼凑至sendBuffer_
				memcpy(sendBuffer, lSendBuffer, sizeof(float) * MONOSENDLENGTH);
				memcpy((sendBuffer + MONOSENDLENGTH), rSendBuffer, sizeof(float) * MONOSENDLENGTH);
				((CA2FFTServer*)pParam)->SendToClients((char*)sendBuffer);
				lData.clear();
				rData.clear();
				lModel.clear();
				rModel.clear();
			}
			audioCapture->ReleaseBuffer();
		}
		else
		{
			Sleep(INTERVAL);
		}
	}
	return 0;
}

bool CA2FFTServer::StartBufferSenderService()
{
	bufferSenderServiceHandle_ = (HANDLE)_beginthreadex(NULL, 0, BufferSenderService,
														this, 0, &bufferSenderServiceID_);
	return (NULL != bufferSenderServiceHandle_) ? true : false;
}

bool CA2FFTServer::StartServer()
{
	bool ret;
	control = true;
	if (!Initial())
	{
		LOG(ERR, L"初始化失败!");
		return false;
	}
	LOG(INFO, L"初始化成功!");
	ret = StartMainLoopService();
	if (!ret)
	{
		LOG(ERR, L"开启主服务失败!");
		return false;
	}
	LOG(INFO, L"开启主服务成功!");
	ret = StartBufferSenderService();
	if (!ret)
	{
		LOG(ERR, L"开启发送服务失败!");
		return false;
	}
	LOG(INFO, L"开启发送服务成功!");
	return true;
}

bool CA2FFTServer::ExitServer()
{
	control = false;
	audioCapture->Stop();
	clientsMutex.lock();
	//关闭所有现有连接
	std::vector<SOCKET>::iterator itr = clientsVector.begin();
	while (itr != clientsVector.end())
	{
		closesocket(*itr);
		itr++;
	}
	clientsVector.clear();
	clientNum = 0;
	CloseHandle(mainLoopServiceHandle_);
	CloseHandle(bufferSenderServiceHandle_);
	clientsMutex.unlock();

	//关闭套接字
	closesocket(socketServer_);
	return true;
}

bool CA2FFTServer::RebootServer()
{
	ExitServer();
	return StartServer();
}