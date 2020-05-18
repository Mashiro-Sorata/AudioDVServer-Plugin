#include "audiocapture.h"


CADataCapture::CADataCapture() : IMMNotificationClient()
{
	pAudioClient = NULL;
	pCaptureClient = NULL;
	pEnumerator = NULL;
	pDevice = NULL;
	pwfx = NULL;
	pData = NULL;
	numFramesAvailable = 0;
	packetLength = 0;
	pData = NULL;
	flags = 0;
	changing_ = false;
	start_ = false;
	wait_ = false;
	role_ = ERole_enum_count;
}

CADataCapture::~CADataCapture()
{
	CoTaskMemFree(pwfx);
	if (pEnumerator != NULL) pEnumerator->Release();
	if (pDevice != NULL) pDevice->Release();
	if (pAudioClient != NULL) pAudioClient->Release();
	if (pCaptureClient != NULL) pCaptureClient->Release();
	CoUninitialize();
	pEnumerator->UnregisterEndpointNotificationCallback(this);
}

HRESULT CADataCapture::Initial()
{

	HRESULT hr;
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to CoInitialize"));
		return hr;
	}

	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to CoCreateInstance"));
		return hr;
	}
	pEnumerator->RegisterEndpointNotificationCallback(this);
	return S_OK;
}

HRESULT CADataCapture::ExInitial()
{
	HRESULT hr;
	// get default output audio endpoint
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetDefaultAudioEndpoint"));
		return hr;
	}

	// activates device
	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to Activate Decive"));
		return hr;
	}

	// gets audio format
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetMixFormat"));
		return hr;
	}

	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, pwfx, NULL);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to Initialize Audio Client"));
		return hr;
	}

	UINT32 bufferFrameCount;
	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetBufferSize"));
		return hr;
	}

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	if (FAILED(hr)) {
		LOG_ERROR(_T("Faild to GetService"));
		return hr;
	}
	return S_OK;
}


HRESULT CADataCapture::Start()
{
	HRESULT hr = pAudioClient->Start();
	if (FAILED(hr))
	{
		LOG_ERROR(_T("Failed to Start"));
		return hr;
	}
	start_ = true;
	return hr;
}

HRESULT CADataCapture::Stop()
{
	 HRESULT hr = pAudioClient->Stop();
	if (FAILED(hr))
	{
		LOG_ERROR(_T("Failed to Stop"));
	}
	start_ = false;
	return hr;
}

HRESULT CADataCapture::get_NextPacketSize()
{
	HRESULT hr;
	hr = pCaptureClient->GetNextPacketSize(&packetLength);
	return hr;
}

HRESULT CADataCapture::get_Buffer()
{
	HRESULT hr;
	hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
	return hr;
}

HRESULT CADataCapture::ReleaseBuffer()
{
	HRESULT hr;
	hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
	return hr;
}

void CADataCapture::WaitBegin()
{
	wait_ = true;
}

void CADataCapture::WaitEnd()
{
	wait_ = false;
}

bool CADataCapture::IsChanging()
{
	return changing_;
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
	return InterlockedIncrement(&mReferenceCount);
}

ULONG STDMETHODCALLTYPE CADataCapture::Release(void)
{
	auto count = InterlockedDecrement(&mReferenceCount);
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
	if (!pEnumerator)
	{
		return S_OK;
	}
	if (role_ == ERole_enum_count)
	{
		role_ = role;
	}
	else
	{
		if (role_ != role)
			return S_OK;
	}
	LOG_INFO(_T("Device Changed!"));
	changing_ = true;
	while (wait_)
	{
		Sleep(1);
	}
	pAudioClient->Stop();
	pAudioClient->Release();
	CoTaskMemFree(pwfx);
	pDevice->Release();

	pEnumerator->GetDevice(pwstrDefaultDeviceId, &pDevice);
	ExInitial();
	if (start_)
	{
		Start();
	}
	changing_ = false;
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