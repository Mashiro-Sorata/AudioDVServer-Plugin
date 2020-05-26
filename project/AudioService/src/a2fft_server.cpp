#include "../include/a2fft_server.h"
#include "../include/AudioFFT.h"
#include "../include/sha1.h"
#include "../include/base64.h"



static struct WebSocketStreamHeader {
	unsigned int header_size;				//数据包头大小
	int mask_offset;					//掩码偏移
	unsigned int payload_size;				//数据大小
	bool fin;                                               //帧标记
	bool masked;					        //掩码
	unsigned char opcode;					//操作码
	unsigned char res[3];
};

static enum WS_FrameType
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
	uint64_t frameSize = static_cast<uint64_t>(frameHeaderSize) + static_cast<uint64_t>(messageLength);
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
//将sm_ComplexSize_个fft数据压缩为SENDLENGTH/2个
const int CA2FFTServer::sm_DataIndex[64] = { 1, 3, 5, 6, 8, 9, 10, 11, 13, 14, 15, 17, 18, 20, 22, 24, 27, 29, 32, 35, 38, 42, 46, 50, 55, 60, 66, 72, 79, 87, 95, 104, 114, 125, 137, 149, 164, 179, 196, 215, 235, 257, 281, 308, 337, 369, 404, 442, 484, 529, 579, 634, 694, 759, 831, 909, 995, 1089, 1192, 1304, 1427, 1562, 1710, 2048 };
const int CA2FFTServer::sm_Gap[64] = { 1, 2, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 2, 3, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 12, 15, 15, 17, 19, 20, 22, 24, 27, 29, 32, 35, 38, 42, 45, 50, 55, 60, 65, 72, 78, 86, 94, 103, 112, 123, 135, 148, 338 };



const u_short CA2FFTServer::sm_Interval = 25;
const u_short CA2FFTServer::sm_SendLength = 128;
const u_short CA2FFTServer::sm_MonoSendLength = CA2FFTServer::sm_SendLength / 2;

std::atomic<bool> CA2FFTServer::sm_control_ = false;
std::vector<SOCKET> CA2FFTServer::sm_clientsVector_;
std::mutex CA2FFTServer::sm_clientsMutex_;
std::atomic<u_short> CA2FFTServer::sm_clientNum_ = 0;

CADataCapture* CA2FFTServer::sm_pAudioCapture_ = NULL;
const UINT32 CA2FFTServer::sm_DataSize_ = 4096;
const size_t CA2FFTServer::sm_ComplexSize_ = audiofft::AudioFFT::ComplexSize(sm_DataSize_);

float* CA2FFTServer::sm_pSendBuffer_ = NULL;
float* CA2FFTServer::sm_pLSendBuffer_ = NULL;
float* CA2FFTServer::sm_pRSendBuffer_ = NULL;


CA2FFTServer::CA2FFTServer(const char* ip, u_short port, int maxClients)
{
	m_ip_ = inet_addr(ip);
	m_port_ = (port < 1 || port > 65535) ? htons(DEFAULT_PORT) : htons(port);
	m_maxClients_ = (maxClients < 1) ? DEFAULT_MAXCLIENTS : maxClients;
	m_socketServer_ = NULL;
	m_mainLoopServiceID_ = NULL;
	sm_clientsVector_.reserve(m_maxClients_);

	m_socketServer_ = NULL;
	//填充服务端信息
	m_serverAddr_.sin_family = AF_INET;
	m_serverAddr_.sin_addr.S_un.S_addr = m_ip_;
	m_serverAddr_.sin_port = m_port_;

	sm_pSendBuffer_ = new float[sm_SendLength];
	sm_pLSendBuffer_ = new float[sm_MonoSendLength];
	sm_pRSendBuffer_ = new float[sm_MonoSendLength];
	sm_pAudioCapture_ = new CADataCapture();

	m_mainLoopServiceHandle_ = NULL;
	m_mainLoopServiceID_ = 0;
	m_bufferSenderServiceHandle_ = NULL;
	m_bufferSenderServiceID_ = 0;
}

CA2FFTServer::CA2FFTServer()
{
	m_ip_ = htonl(INADDR_LOOPBACK);
	m_port_ = htons(DEFAULT_PORT);
	m_maxClients_ = DEFAULT_MAXCLIENTS;
	m_socketServer_ = NULL;
	m_mainLoopServiceID_ = NULL;
	sm_clientsVector_.reserve(m_maxClients_);

	m_socketServer_ = NULL;
	//填充服务端信息
	m_serverAddr_.sin_family = AF_INET;
	m_serverAddr_.sin_addr.S_un.S_addr = m_ip_;
	m_serverAddr_.sin_port = m_port_;

	sm_pSendBuffer_ = new float[sm_SendLength];
	sm_pLSendBuffer_ = new float[sm_MonoSendLength];
	sm_pRSendBuffer_ = new float[sm_MonoSendLength];
	sm_pAudioCapture_ = new CADataCapture();

	m_mainLoopServiceHandle_ = NULL;
	m_mainLoopServiceID_ = 0;
	m_bufferSenderServiceHandle_ = NULL;
	m_bufferSenderServiceID_ = 0;
}

CA2FFTServer::~CA2FFTServer()
{
	ExitServer();
	delete[] sm_pSendBuffer_;
	delete[] sm_pLSendBuffer_;
	delete[] sm_pRSendBuffer_;
	delete sm_pAudioCapture_;
	sm_pAudioCapture_ = NULL;
	sm_pSendBuffer_ = NULL;
	sm_pLSendBuffer_ = NULL;
	sm_pRSendBuffer_ = NULL;
	//释放DLL资源
	WSACleanup();
}

bool CA2FFTServer::Initial_()
{
	//版本号
	WORD w_req = MAKEWORD(2, 2);
	WSADATA wsadata;

	//初始化Windows Sockets DLL
	if (WSAStartup(w_req, &wsadata) != 0) {
		LOG_ERROR("初始化套接字库失败!");
		return false;
	}
	else
	{
		LOG_INFO("初始化套接字库成功!");
	}

	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		LOG_WARN("套接字库版本号不符!");
	}
	else
	{
		LOG_INFO("套接字库版本正确!");
	}

	//创建套接字
	m_socketServer_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	BOOL bReUseAddr = TRUE;
	setsockopt(m_socketServer_, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReUseAddr, sizeof(BOOL));
	BOOL  bDontLinger = FALSE;
	setsockopt(m_socketServer_, SOL_SOCKET, SO_DONTLINGER, (const char*)&bDontLinger, sizeof(BOOL));

	if (bind(m_socketServer_, (SOCKADDR*)&m_serverAddr_, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		LOG_ERROR("套接字绑定失败!");
		WSACleanup();
		return false;
	}
	else
	{
		LOG_INFO("套接字绑定成功!");
	}

	//设置套接字为监听状态
	if (listen(m_socketServer_, SOMAXCONN) < 0)
	{
		LOG_ERROR("设置监听状态失败!");
		WSACleanup();
		return false;
	}
	else
	{
		LOG_INFO("设置监听状态成功!");
	}

	if (FAILED(sm_pAudioCapture_->Initial()))
	{
		LOG_ERROR("初始化CADataCapture失败!");
		return false;
	}
	LOG_INFO("初始化CADataCapture成功!");

	if (sm_pAudioCapture_->StartExInitialService())
	{
		LOG_INFO("CADataCapture::ExInitial成功!");
		return true;
	}
	else
	{
		LOG_ERROR("CADataCapture::ExInitial失败!");
		return false;
	}
}

unsigned int __stdcall CA2FFTServer::MainLoopService_(PVOID pParam)
{
	int skAddrLength = sizeof(SOCKADDR);
	int len;
	char buff[1024];
	std::string strout;
	SOCKADDR_IN acceptAddr;
	SOCKET socketClient;
	while (sm_control_)
	{
		if (sm_clientNum_ < ((CA2FFTServer*)pParam)->m_maxClients_)
		{
			socketClient = accept(((CA2FFTServer*)pParam)->m_socketServer_,
				(SOCKADDR*)&acceptAddr, &skAddrLength);
			if (socketClient == SOCKET_ERROR)
			{
				LOG_WARN("建立连接失败!");
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
						if (sm_clientNum_ == 0)
						{
							sm_pAudioCapture_->Start();
						}
						sm_clientNum_++;
						sm_clientsMutex_.lock();
						sm_clientsVector_.push_back(socketClient);
						sm_clientsMutex_.unlock();
						LOG_INFO("客户端连接成功!");
					}
					else
					{
						LOG_WARN("WebSocket握手失败!");
					}
				}
				else
				{
					LOG_WARN("连接时数据接受失败!");
				}
			}
		}
	}
	return 0;
}

bool CA2FFTServer::StartMainLoopService_()
{
	m_mainLoopServiceHandle_ = (HANDLE)_beginthreadex(NULL, 0, MainLoopService_, this, 0, &m_mainLoopServiceID_);
	return (NULL != m_mainLoopServiceHandle_) ? true : false;
}

void CA2FFTServer::SendToClients_(char* buffer)
{
	int send_len = 0;
	sm_clientsMutex_.lock();
	std::vector<SOCKET>::iterator itr = sm_clientsVector_.begin();
	while (itr != sm_clientsVector_.end())
	{
		send_len = 0;
		send_len = wsSend(*itr, buffer, sm_SendLength * sizeof(float));
		if (send_len < 0)
		{
			//客户端断开连接
			LOG_WARN("客户端断开连接!");
			if (sm_clientNum_ == 1)
			{
				sm_pAudioCapture_->Stop();
			}
			sm_clientNum_--;
			closesocket(*itr);
			itr = sm_clientsVector_.erase(itr);
		}
		else
		{
			itr++;
		}
	}
	sm_clientsMutex_.unlock();
}


unsigned int __stdcall CA2FFTServer::BufferSenderService_(PVOID pParam)
{
	//将buff区以float的形式读取
	float* pfData = NULL;
	//数据包定位符，将数据包的数据凑满sm_DataSize_个后进行处理
	UINT32 desPtn = 0;
	UINT32 srcPtn = 0;
	INT32 packRem = 0;
	audiofft::AudioFFT fft;
	fft.init(sm_DataSize_);
	std::vector<float> lData;
	std::vector<float> rData;
	std::vector<float> lRe;
	std::vector<float> lImg;
	std::vector<float> rRe;
	std::vector<float> rImg;
	lData.reserve(sm_DataSize_);
	rData.reserve(sm_DataSize_);
	lRe.resize(sm_ComplexSize_);
	lImg.resize(sm_ComplexSize_);
	rRe.resize(sm_ComplexSize_);
	rImg.resize(sm_ComplexSize_);

	//fft幅值
	std::vector<float> lModel;
	std::vector<float> rModel;
	lModel.reserve(sm_ComplexSize_);
	rModel.reserve(sm_ComplexSize_);
	//临时变量
	unsigned int j = 0;
	float lSum = 0.0;
	float rSum = 0.0;

	unsigned int errCount = 0;

	//循环更新数据后发送的操作
	HRESULT hr;
	while (sm_control_)
	{
		//当有客户端连接时采集音频数据处理或默认音频设备未发生改变时采集音频数据处理
		if ((sm_clientNum_ > 0) && (!sm_pAudioCapture_->IsChanging()))
		{
			sm_pAudioCapture_->sm_mutexWait.lock();
			hr = sm_pAudioCapture_->GetNextPacketSize();
			if (!FAILED(hr) && (0 != sm_pAudioCapture_->GetPacketLength()))
			{
				hr = sm_pAudioCapture_->GetBuffer(&pfData);
				if (!FAILED(hr))
				{
					packRem = desPtn + (sm_pAudioCapture_->GetNumFramesAvailable()) - sm_DataSize_;
					if (packRem < 0)
					{
						for (unsigned int i = 0; i < sm_pAudioCapture_->GetNumFramesAvailable() * 2; i += 2)
						{
							lData.push_back(*(pfData + i));
							rData.push_back(*(pfData + i + 1));
						}
						desPtn += sm_pAudioCapture_->GetNumFramesAvailable();
						srcPtn = 0;
					}
					else if (packRem > 0)
					{
						while (TRUE)
						{
							for (unsigned int i = 0; i < (sm_DataSize_ - desPtn) * 2; i += 2)
							{
								lData.push_back(*(pfData + srcPtn + i));
								rData.push_back(*(pfData + srcPtn + i + 1));
							}
							srcPtn += sm_DataSize_ - desPtn;
							desPtn = 0;
							fft.fft(&lData[0], &lRe[0], &lImg[0]);
							fft.fft(&rData[0], &rRe[0], &rImg[0]);
							//数据压缩处理，非线性段求均值，单声道压缩至64个数据
							j = 0;
							for (unsigned int i = 0; i < sm_MonoSendLength; i++)
							{
								lSum = 0.0;
								rSum = 0.0;
								while (j < sm_DataIndex[i])
								{
									if (j > sm_ComplexSize_ - 1)
									{
										sm_control_ = false;
										return 1;
									}
									//取模
									lModel.push_back(sqrt(lRe[j] * lRe[j] + lImg[j] * lImg[j]));
									rModel.push_back(sqrt(rRe[j] * rRe[j] + rImg[j] * rImg[j]));
									lSum += lModel.back();
									rSum += rModel.back();
									j++;
								}
								sm_pLSendBuffer_[i] = lSum / sm_Gap[i];
								sm_pRSendBuffer_[i] = rSum / sm_Gap[i];
							}
							//将压缩的数据拼凑至sendBuffer_
							memcpy(sm_pSendBuffer_, sm_pLSendBuffer_, sizeof(float) * sm_MonoSendLength);
							memcpy((sm_pSendBuffer_ + sm_MonoSendLength), sm_pRSendBuffer_, sizeof(float) * sm_MonoSendLength);
							((CA2FFTServer*)pParam)->SendToClients_((char*)sm_pSendBuffer_);
							lData.clear();
							rData.clear();
							lModel.clear();
							rModel.clear();
							if (sm_DataSize_ < packRem)
							{
								packRem -= sm_DataSize_;
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
						for (unsigned int i = 0; i < (sm_pAudioCapture_->GetNumFramesAvailable()) * 2; i += 2)
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
						for (unsigned int i = 0; i < sm_MonoSendLength; i++)
						{
							lSum = 0.0;
							rSum = 0.0;
							while (j < sm_DataIndex[i])
							{
								if (j > sm_ComplexSize_ - 1)
								{
									sm_control_ = false;
									return 1;
								}
								//取模
								lModel.push_back(sqrt(lRe[j] * lRe[j] + lImg[j] * lImg[j]));
								rModel.push_back(sqrt(rRe[j] * rRe[j] + rImg[j] * rImg[j]));
								lSum += lModel.back();
								rSum += rModel.back();
								j++;
							}
							sm_pLSendBuffer_[i] = lSum / sm_Gap[i];
							sm_pRSendBuffer_[i] = rSum / sm_Gap[i];
						}
						//将压缩的数据拼凑至sendBuffer_
						memcpy(sm_pSendBuffer_, sm_pLSendBuffer_, sizeof(float) * sm_MonoSendLength);
						memcpy((sm_pSendBuffer_ + sm_MonoSendLength), sm_pRSendBuffer_, sizeof(float) * sm_MonoSendLength);
						((CA2FFTServer*)pParam)->SendToClients_((char*)sm_pSendBuffer_);
						lData.clear();
						rData.clear();
						lModel.clear();
						rModel.clear();
					}
				}
				else
				{
					if (10 == errCount)
					{
						sm_pAudioCapture_->ReStart();
						errCount = 0;
					}
					else
					{
						++errCount;
						switch (hr)
						{
						case(AUDCLNT_S_BUFFER_EMPTY):
							LOG_WARN("AUDCLNT_S_BUFFER_EMPTY(没有捕获数据可读取)");
							break;
						case(AUDCLNT_E_BUFFER_ERROR):
							LOG_WARN("AUDCLNT_E_BUFFER_ERROR(无法检索数据缓冲区,指针指向NULL)");
							sm_pAudioCapture_->Stop();
							sm_pAudioCapture_->Reset();
							sm_pAudioCapture_->Start();
							break;
						case(AUDCLNT_E_OUT_OF_ORDER):
							LOG_WARN("AUDCLNT_E_OUT_OF_ORDER(未释放数据缓冲区,先前的GetBuffer调用仍然有效)");
							break;
						case(AUDCLNT_E_DEVICE_INVALIDATED):
							LOG_WARN("AUDCLNT_E_DEVICE_INVALIDATED(音频终结点设备已拔出,或者音频硬件或关联的硬件资源已重新配置,禁用,删除或以其他方式变得无法使用)");
							break;
						case(AUDCLNT_E_BUFFER_OPERATION_PENDING):
							LOG_WARN("AUDCLNT_E_BUFFER_OPERATION_PENDING(由于正在进行流重置,因此无法访问缓冲区)");
							break;
						case(AUDCLNT_E_SERVICE_NOT_RUNNING):
							LOG_WARN("AUDCLNT_E_SERVICE_NOT_RUNNING(Windows音频服务未运行)");
							break;
						case(E_POINTER):
							LOG_WARN("E_POINTER(参数ppData,pNumFramesToRead或pdwFlags为NULL)");
							break;
						default:
							LOG_WARN("E_UNKNOW(获取数据缓冲区时发生未知错误)");
						}
						Sleep(sm_Interval);
					}
				}
				sm_pAudioCapture_->ReleaseBuffer();
				pfData = NULL;
			}
			sm_pAudioCapture_->sm_mutexWait.unlock();
		}
		Sleep(sm_Interval);
	}
	return 0;
}

bool CA2FFTServer::StartBufferSenderService_()
{
	m_bufferSenderServiceHandle_ = (HANDLE)_beginthreadex(NULL, 0, BufferSenderService_,
		this, 0, &m_bufferSenderServiceID_);
	return (NULL != m_bufferSenderServiceHandle_) ? true : false;
}

bool CA2FFTServer::StartServer()
{
	bool ret;
	sm_control_ = true;
	if (!Initial_())
	{
		LOG_ERROR("初始化失败!");
		return false;
	}
	LOG_INFO("初始化成功!");
	ret = StartMainLoopService_();
	if (!ret)
	{
		LOG_ERROR("开启主服务失败!");
		return false;
	}
	LOG_INFO("开启主服务成功!");
	ret = StartBufferSenderService_();
	if (!ret)
	{
		LOG_ERROR("开启发送服务失败!");
		return false;
	}
	LOG_INFO("开启发送服务成功!");
	return true;
}

bool CA2FFTServer::ExitServer()
{
	sm_control_ = false;
	DWORD _ret = WaitForSingleObject(m_bufferSenderServiceHandle_, 5000);
	if (WAIT_OBJECT_0 != _ret)
	{
		LOG_WARN("强制关闭BufferSenderService");
		CloseHandle(m_bufferSenderServiceHandle_);
	}
	sm_pAudioCapture_->Stop();

	sm_clientsMutex_.lock();
	//关闭所有现有连接
	std::vector<SOCKET>::iterator itr = sm_clientsVector_.begin();
	while (itr != sm_clientsVector_.end())
	{
		closesocket(*itr);
		itr++;
	}
	sm_clientsVector_.clear();
	sm_clientNum_ = 0;
	CloseHandle(m_mainLoopServiceHandle_);
	sm_clientsMutex_.unlock();

	//关闭套接字
	closesocket(m_socketServer_);
	return true;
}

bool CA2FFTServer::RebootServer()
{
	ExitServer();
	return StartServer();
}