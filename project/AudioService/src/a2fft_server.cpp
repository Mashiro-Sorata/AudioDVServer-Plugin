#include "../include/a2fft_server.h"
#include "../include/AudioFFT.h"
#include "../include/sha1.h"
#include "../include/base64.h"


//---------------WebSocket服务函数------------------

/*
1.握手。
client第一次connet连接会发起握手协议，server在recv接收处解析，
判断如果是websocket的握手协议，那么同样组装好特定格式包头回复给client，建立连接。
*/
//判断是不是websocket协议
static bool isWSHandShake(std::string& request)
{
	size_t i = request.find("GET");
	if (i == std::string::npos) {
		return false;
	}
	return true;
}

//如果是，解析握手协议重新组装准备send回复给client
static bool wsHandshake(std::string& request, std::string& response)
{
	//得到客户端请求信息的key
	std::string tempKey = request;
	size_t i = tempKey.find("Sec-WebSocket-Key");
	if (i == std::string::npos) {
		return false;
	}
	tempKey = tempKey.substr(i + 19, 24);

	//拼接协议返回给客户端
	response = "HTTP/1.1 101 Switching Protocols\r\n";
	response += "Connection: upgrade\r\n";
	response += "Sec-WebSocket-Accept: ";

	std::string realKey = tempKey;
	realKey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1 sha;
	unsigned int message_digest[5];
	sha.Reset();
	sha << realKey.c_str();
	sha.Result(message_digest);
	for (int i = 0; i < 5; i++) {
		message_digest[i] = htonl(message_digest[i]);
	}
	realKey = base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);
	realKey += "\r\n";
	response += realKey.c_str();
	response += "Upgrade: websocket\r\n\r\n";
	return true;
}

/*
2.接收client协议解析
首先解析包头信息
*/
static bool wsReadHeader(char* cData, WebSocketStreamHeader* header)
{
	if (cData == NULL) return false;

	const char* buf = cData;
	header->fin = buf[0] & 0x80;
	header->masked = buf[1] & 0x80;
	unsigned char stream_size = buf[1] & 0x7F;

	header->opcode = buf[0] & 0x0F;
	// if (header->opcode == WS_FrameType::WS_CONTINUATION_FRAME) {  
	//     //连续帧  
	//     return false;  
	// }
	if (header->opcode == WS_TEXT_FRAME) {
		//文本帧  
	}
	else if (header->opcode == WS_BINARY_FRAME) {
		//二进制帧
	}
	else if (header->opcode == WS_CLOSING_FRAME) {
		//连接关闭消息  
		return true;
	}
	else if (header->opcode == WS_PING_FRAME) {
		//  ping  
		return false;
	}
	else if (header->opcode == WS_PONG_FRAME) {
		// pong  
		return false;
	}
	else {
		//非法帧  
		return false;
	}

	if (stream_size <= 125) {
		//  small stream  
		header->header_size = 6;
		header->payload_size = stream_size;
		header->mask_offset = 2;
	}
	else if (stream_size == 126) {
		//  medium stream   
		header->header_size = 8;
		unsigned short s = 0;
		memcpy(&s, (const char*)&buf[2], 2);
		header->payload_size = ntohs(s);
		header->mask_offset = 4;
	}
	else if (stream_size == 127) {

		unsigned long long l = 0;
		memcpy(&l, (const char*)&buf[2], 8);

		header->payload_size = l;
		header->mask_offset = 10;
	}
	else {
		//Couldnt decode stream size 非法大小数据包  
		return false;
	}

	/*  if (header->payload_size > MAX_WEBSOCKET_BUFFER) {
	return false;
	} */

	return true;
}

//然后根据包头解析出真实数据
static bool wsDecodeFrame(WebSocketStreamHeader* header, char cbSrcData[], unsigned short wSrcLen, char cbTagData[])
{
	const  char* final_buf = cbSrcData;
	if (wSrcLen < header->header_size + 1) {
		return false;
	}

	char masks[4];
	memcpy(masks, final_buf + header->mask_offset, 4);
	memcpy(cbTagData, final_buf + header->mask_offset + 4, header->payload_size);

	for (unsigned int i = 0; i < header->payload_size; ++i) {
		cbTagData[i] = (cbTagData[i] ^ masks[i % 4]);
	}
	//如果是文本包，在数据最后加一个结束字符“\0”
	if (header->opcode == WS_TEXT_FRAME)
		cbTagData[header->payload_size] = '\0';

	return true;
}

//3.组装server发给client协议
static bool wsEncodeFrame(std::string inMessage, std::string& outFrame, enum WS_FrameType frameType)
{
	const uint32_t messageLength = inMessage.size();
	if (messageLength > 32767)
	{
		// 暂不支持这么长的数据  
		return false;
	}

	uint8_t payloadFieldExtraBytes = (messageLength <= 0x7d) ? 0 : 2;
	// header: 2字节, mask位设置为0(不加密), 则后面的masking key无须填写, 省略4字节  
	uint8_t frameHeaderSize = 2 + payloadFieldExtraBytes;
	uint8_t* frameHeader = new uint8_t[frameHeaderSize];
	memset(frameHeader, 0, frameHeaderSize);

	// fin位为1, 扩展位为0, 操作位为frameType  
	frameHeader[0] = static_cast<uint8_t>(0x80 | frameType);

	// 填充数据长度
	if (messageLength <= 0x7d)
	{
		frameHeader[1] = static_cast<uint8_t>(messageLength);
	}
	else
	{
		frameHeader[1] = 0x7e;
		uint16_t len = htons(messageLength);
		memcpy(&frameHeader[2], &len, payloadFieldExtraBytes);
	}

	// 填充数据  
	uint32_t frameSize = frameHeaderSize + messageLength;
	char* frame = new char[frameSize + 1];
	memcpy(frame, frameHeader, frameHeaderSize);
	memcpy(frame + frameHeaderSize, inMessage.c_str(), messageLength);
	outFrame = std::string(frame, frameSize);
	delete[] frame;
	delete[] frameHeader;
	return true;
}

static char* wsEncodeFrameBytes(char* inMessage, enum WS_FrameType frameType, uint32_t* length)
{
	uint32_t messageLength;
	if (*length == 0)
		messageLength = strlen(inMessage) + 1;
	else
		messageLength = *length;
	if (messageLength > 32767)
	{
		// 暂不支持这么长的数据  
		return NULL;
	}
	uint8_t payloadFieldExtraBytes = (messageLength <= 0x7d) ? 0 : 2;
	// header: 2字节, mask位设置为0(不加密), 则后面的masking key无须填写, 省略4字节  
	uint8_t frameHeaderSize = 2 + payloadFieldExtraBytes;
	uint8_t* frameHeader = new uint8_t[frameHeaderSize];
	memset(frameHeader, 0, frameHeaderSize);

	// fin位为1, 扩展位为0, 操作位为frameType  
	frameHeader[0] = static_cast<uint8_t>(0x80 | frameType);

	// 填充数据长度
	if (messageLength <= 0x7d)
	{
		frameHeader[1] = static_cast<uint8_t>(messageLength);
	}
	else
	{
		frameHeader[1] = 0x7e;
		uint16_t len = htons(messageLength);
		memcpy(&frameHeader[2], &len, payloadFieldExtraBytes);
	}

	// 填充数据  
	uint32_t frameSize = frameHeaderSize + messageLength;
	char* frame = new char[frameSize + 1];
	memcpy(frame, frameHeader, frameHeaderSize);
	memcpy(frame + frameHeaderSize, inMessage, messageLength);
	*length = frameSize;
	delete[] frameHeader;
	return frame;
}

static int wsRead(SOCKET client, char* data, uint32_t len)
{
	if (SOCKET_ERROR != client)
	{
		char* buff = new char[len];
		len = recv(client, buff, len, 0);
		if (len > 0)
		{
			WebSocketStreamHeader header;
			wsReadHeader(buff, &header);
			wsDecodeFrame(&header, buff, len, data);
		}
		delete[] buff;
		return len;
	}
	return 0;
}

static int wsSend(SOCKET client, char* data, uint32_t len)
{
	int flag;
	uint32_t length;
	char* psend;
	if (SOCKET_ERROR != client)
	{
		if (len == 0)
		{
			length = strlen(data);
			psend = wsEncodeFrameBytes(data, WS_TEXT_FRAME, &length);
		}
		else
		{
			length = len;
			psend = wsEncodeFrameBytes(data, WS_BINARY_FRAME, &length);
		}
		flag = send(client, psend, length, 0);
		delete psend;
		return flag;
	}
	return -1;
}


//--------------CA2FFTServer类---------------------

//与SENDLENGTH相关
//将complexSize个fft数据压缩为SENDLENGTH/2个
const int CA2FFTServer::DataIndex[64] = { 1, 3, 5, 6, 8, 9, 10, 11, 13, 14, 15, 17, 18, 20, 22, 24, 27, 29, 32, 35, 38, 42, 46, 50, 55, 60, 66, 72, 79, 87, 95, 104, 114, 125, 137, 149, 164, 179, 196, 215, 235, 257, 281, 308, 337, 369, 404, 442, 484, 529, 579, 634, 694, 759, 831, 909, 995, 1089, 1192, 1304, 1427, 1562, 1710, 2048 };
const int CA2FFTServer::Gap[64] = { 1, 2, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 2, 3, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 12, 15, 15, 17, 19, 20, 22, 24, 27, 29, 32, 35, 38, 42, 45, 50, 55, 60, 65, 72, 78, 86, 94, 103, 112, 123, 135, 148, 338 };



const u_short CA2FFTServer::INTERVAL = 25;
const u_short CA2FFTServer::SENDLENGTH = 128;
const u_short CA2FFTServer::MONOSENDLENGTH = CA2FFTServer::SENDLENGTH / 2;

std::atomic<bool> CA2FFTServer::control = false;
std::vector<SOCKET> CA2FFTServer::clientsVector;
std::mutex CA2FFTServer::clientsMutex;
std::atomic<u_short> CA2FFTServer::clientNum = 0;

CADataCapture* CA2FFTServer::audioCapture = NULL;
const UINT32 CA2FFTServer::dataSize = 4096;
const size_t CA2FFTServer::complexSize = audiofft::AudioFFT::ComplexSize(dataSize);

float* CA2FFTServer::sendBuffer = NULL;
float* CA2FFTServer::lSendBuffer = NULL;
float* CA2FFTServer::rSendBuffer = NULL;


CA2FFTServer::CA2FFTServer(const char* ip, u_short port, int maxClients)
{
	ip_ = inet_addr(ip);
	port_ = (port < 1 || port > 65535) ? htons(DEFAULT_PORT) : htons(port);
	maxClients_ = (maxClients < 1) ? DEFAULT_MAXCLIENTS : maxClients;
	socketServer_ = NULL;
	mainLoopServiceID_ = NULL;
	sendBuffer = new float[SENDLENGTH];
	lSendBuffer = new float[MONOSENDLENGTH];
	rSendBuffer = new float[MONOSENDLENGTH];
	clientsVector.reserve(maxClients_);

	audioCapture = new CADataCapture();
}

CA2FFTServer::CA2FFTServer()
{
	ip_ = htonl(INADDR_LOOPBACK);
	port_ = htons(DEFAULT_PORT);
	maxClients_ = DEFAULT_MAXCLIENTS;
	socketServer_ = NULL;
	mainLoopServiceID_ = NULL;
	sendBuffer = new float[SENDLENGTH];
	lSendBuffer = new float[MONOSENDLENGTH];
	rSendBuffer = new float[MONOSENDLENGTH];
	clientsVector.reserve(maxClients_);

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
		LOG_ERROR(_T("初始化套接字库失败!"));
		return false;
	}
	else
	{
		LOG_INFO(_T("初始化套接字库成功!"));
	}
	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		LOG_WARN(_T("套接字库版本号不符!"));
	}
	else
	{
		LOG_INFO(_T("套接字库版本正确!"));
	}
	//填充服务端信息
	serverAddr_.sin_family = AF_INET;
	serverAddr_.sin_addr.S_un.S_addr = ip_;
	serverAddr_.sin_port = port_;
	//创建套接字
	socketServer_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//接受连接请求

	BOOL bReUseAddr = TRUE;
	setsockopt(socketServer_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReUseAddr, sizeof(BOOL));
	BOOL  bDontLinger = FALSE;
	setsockopt(socketServer_, SOL_SOCKET, SO_DONTLINGER, (const char*)&bDontLinger, sizeof(BOOL));
	if (bind(socketServer_, (SOCKADDR*)&serverAddr_, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		LOG_ERROR(_T("套接字绑定失败!"));
		WSACleanup();
		return false;
	}
	else
	{
		LOG_INFO(_T("套接字绑定成功!"));
	}

	//设置套接字为监听状态
	if (listen(socketServer_, SOMAXCONN) < 0)
	{
		LOG_ERROR(_T("设置监听状态失败!"));
		WSACleanup();
		return false;
	}
	else
	{
		LOG_INFO(_T("设置监听状态成功!"));
	}
	
	if (FAILED(audioCapture->Initial()))
	{
		LOG_ERROR(_T("初始化CADataCapture失败!"));
		return false;
	}
	LOG_INFO(_T("初始化CADataCapture成功!"));

	if (FAILED(audioCapture->ExInitial()))
	{
		LOG_ERROR(_T("CADataCapture::ExInitial失败!"));
		return false;
	}
	LOG_INFO(_T("CADataCapture::ExInitial成功!"));

	return true;
}

unsigned int __stdcall CA2FFTServer::MainLoopService(PVOID pParam)
{
	int skAddrLength = sizeof(SOCKADDR);
	int len;
	char buff[1024];
	std::string strout;
	SOCKADDR_IN acceptAddr;
	SOCKET socketClient;
	while (control)
	{
		if (clientNum < ((CA2FFTServer*)pParam)->maxClients_)
		{
			socketClient = accept(((CA2FFTServer*)pParam)->socketServer_,
				(SOCKADDR*)&acceptAddr, &skAddrLength);
			if (socketClient == SOCKET_ERROR)
			{
				LOG_WARN(_T("尝试连接失败!"));
			}
			else
			{
				len = recv(socketClient, buff, 1024, 0);
				if (len > 0)
				{
					std::string str = buff;
					if (isWSHandShake(str) == true)
					{
						wsHandshake(str, strout);
						send(socketClient, (char*)(strout.c_str()), strout.size(), 0);
						if (clientNum == 0)
						{
							audioCapture->Start();
							audioCapture->start = true;
						}
						clientNum++;
						clientsMutex.lock();
						clientsVector.push_back(socketClient);
						clientsMutex.unlock();
						LOG_INFO(_T("连接成功!"));
					}
					else
					{
						LOG_WARN(_T("WebSocket握手失败!"));
					}
				}
				else
				{
					LOG_WARN(_T("数据接受失败!"));
				}
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
		send_len = wsSend(*itr, buffer, SENDLENGTH * sizeof(float));
		if (send_len < 0)
		{
			//客户端断开连接
			LOG_WARN(_T("断开连接!"));
			if (clientNum == 1)
			{
				audioCapture->Stop();
				audioCapture->start = false;
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
	//将buff区以float的形式读取
	float* pfData;
	//数据包定位符，将数据包的数据凑满dataSize个后进行处理
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
			if (!audioCapture->changing)
			{
				audioCapture->wait = true;
				//发送缓冲区数据给所有的client
				hr = audioCapture->get_NextPacketSize();
				if (audioCapture->packetLength == 0)
				{
					Sleep(INTERVAL);
					continue;
				}
				hr = audioCapture->get_Buffer();
				packRem = desPtn + (audioCapture->numFramesAvailable) - dataSize;
				pfData = (float*)(audioCapture->pData);
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
				audioCapture->wait = false;
				Sleep(INTERVAL);
			}
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
		LOG_ERROR(_T("初始化失败!"));
		return false;
	}
	LOG_INFO(_T("初始化成功!"));
	ret = StartMainLoopService();
	if (!ret)
	{
		LOG_ERROR(_T("开启主服务失败!"));
		return false;
	}
	LOG_INFO(_T("开启主服务成功!"));
	ret = StartBufferSenderService();
	if (!ret)
	{
		LOG_ERROR(_T("开启发送服务失败!"));
		return false;
	}
	LOG_INFO(_T("开启发送服务成功!"));
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