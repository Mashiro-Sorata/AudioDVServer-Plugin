#include "audiocapture.h"
#include "debug.h"


const IID IID_IAudioCaptureClient(__uuidof(IAudioCaptureClient));
const CLSID CLSID_MMDeviceEnumerator(__uuidof(MMDeviceEnumerator));
const IID IID_IMMDeviceEnumerator(__uuidof(IMMDeviceEnumerator));
const IID IID_IAudioClient(__uuidof(IAudioClient));

CADataCapture::CADataCapture() : IMMNotificationClient()
{
	m_pAudioClient_ = NULL;
	m_pCaptureClient_ = NULL;
	m_pEnumerator_ = NULL;
	m_pDevice_ = NULL;
	m_pwfx_ = NULL;
	m_pData_ = NULL;
	m_numFramesAvailable_ = 0;
	m_packetLength_ = 0;
	m_pData_ = NULL;
	m_flags_ = 0;
	m_changing_ = false;
	m_start_ = false;
	m_wait_ = false;
	m_role_ = ERole_enum_count;
}

CADataCapture::~CADataCapture()
{
	m_pEnumerator_->UnregisterEndpointNotificationCallback(this);
	CoTaskMemFree(m_pwfx_);
	if (m_pCaptureClient_ != NULL) m_pCaptureClient_->Release();
	if (m_pAudioClient_ != NULL) m_pAudioClient_->Release();
	if (m_pDevice_ != NULL) m_pDevice_->Release();
	if (m_pEnumerator_ != NULL) m_pEnumerator_->Release();
	m_pData_ = NULL;
	LOG_INFO("~CADataCapture!");
	CoUninitialize();
}

HRESULT CADataCapture::Initial()
{

	HRESULT hr;
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to CoInitialize"));
		return hr;
	}

	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&m_pEnumerator_);

	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to CoCreateInstance"));
		return hr;
	}
	m_pEnumerator_->RegisterEndpointNotificationCallback(this);
	return S_OK;
}

HRESULT CADataCapture::ExInitial()
{
	HRESULT hr;
	// get default output audio endpoint
	hr = m_pEnumerator_->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_pDevice_);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetDefaultAudioEndpoint"));
		return hr;
	}

	// activates device
	hr = m_pDevice_->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient_);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to Activate Decive"));
		return hr;
	}

	// gets audio format
	hr = m_pAudioClient_->GetMixFormat(&m_pwfx_);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetMixFormat"));
		return hr;
	}

	hr = m_pAudioClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, m_pwfx_, NULL);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to Initialize Audio Client"));
		return hr;
	}

	UINT32 bufferFrameCount;
	// Get the size of the allocated buffer.
	hr = m_pAudioClient_->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetBufferSize"));
		return hr;
	}

	hr = m_pAudioClient_->GetService(IID_IAudioCaptureClient, (void**)&m_pCaptureClient_);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetService"));
		return hr;
	}
	return S_OK;
}


HRESULT CADataCapture::Start()
{
	HRESULT hr = m_pAudioClient_->Start();
	if (FAILED(hr))
	{
		LOG_ERROR(_T("Failed to Start"));
		return hr;
	}
	m_start_ = true;
	return hr;
}

HRESULT CADataCapture::Stop()
{
	 HRESULT hr = m_pAudioClient_->Stop();
	if (FAILED(hr))
	{
		LOG_ERROR(_T("Failed to Stop"));
	}
	m_start_ = false;
	return hr;
}

HRESULT CADataCapture::get_NextPacketSize()
{
	HRESULT hr;
	hr = m_pCaptureClient_->GetNextPacketSize(&m_packetLength_);
	return hr;
}

HRESULT CADataCapture::get_Buffer()
{
	HRESULT hr;
	hr = m_pCaptureClient_->GetBuffer(&m_pData_, &m_numFramesAvailable_, &m_flags_, NULL, NULL);
	return hr;
}

HRESULT CADataCapture::ReleaseBuffer()
{
	HRESULT hr;
	hr = m_pCaptureClient_->ReleaseBuffer(m_numFramesAvailable_);
	return hr;
}

void CADataCapture::WaitBegin()
{
	m_wait_ = true;
}

void CADataCapture::WaitEnd()
{
	m_wait_ = false;
}

bool CADataCapture::IsChanging()
{
	return m_changing_;
}

UINT32 CADataCapture::GetNumFramesAvailable()
{
	return m_numFramesAvailable_;
}

UINT32 CADataCapture::GetPacketLength()
{
	return m_packetLength_;
}

DWORD CADataCapture::GetFlags()
{
	return m_flags_;
}

void CADataCapture::GetData(float** dataBuff)
{
	*dataBuff = (float*)m_pData_;
}

HRESULT STDMETHODCALLTYPE CADataCapture::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObject = static_cast<IUnknown*>(this);
	}
	else if (riid == __uuidof(IMMNotificationClient))
	{
		AddRef();
		*ppvObject = static_cast<IMMNotificationClient*>(this);
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	return S_OK;
}

ULONG STDMETHODCALLTYPE CADataCapture::AddRef(void)
{
	return InterlockedIncrement(&m_referenceCount_);
}

ULONG STDMETHODCALLTYPE CADataCapture::Release(void)
{
	auto count = InterlockedDecrement(&m_referenceCount_);
	if (count == 0)
	{
		delete this;
	}
	return count;
}

HRESULT STDMETHODCALLTYPE CADataCapture::OnDefaultDeviceChanged(
	_In_ EDataFlow flow,
	_In_ ERole role,
	_In_ LPCWSTR pwstrDefaultDeviceId
)
{
	if (!m_pEnumerator_)
	{
		return S_OK;
	}
	if (m_role_ == ERole_enum_count)
	{
		m_role_ = role;
	}
	else
	{
		if (m_role_ != role)
			return S_OK;
	}
	LOG_INFO(_T("Device Changed!"));
	m_changing_ = true;
	while (m_wait_)
	{
		Sleep(1);
	}
	m_pAudioClient_->Stop();
	m_pAudioClient_->Release();
	CoTaskMemFree(m_pwfx_);
	m_pDevice_->Release();

	m_pEnumerator_->GetDevice(pwstrDefaultDeviceId, &m_pDevice_);
	ExInitial();
	if (m_start_)
	{
		Start();
	}
	m_changing_ = false;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CADataCapture::OnDeviceStateChanged(_In_  LPCWSTR pwstrDeviceId, _In_  DWORD dwNewState)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CADataCapture::OnDeviceAdded(_In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CADataCapture::OnDeviceRemoved(_In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CADataCapture::OnPropertyValueChanged(_In_  LPCWSTR pwstrDeviceId, _In_  const PROPERTYKEY key)
{
	return S_OK;
}