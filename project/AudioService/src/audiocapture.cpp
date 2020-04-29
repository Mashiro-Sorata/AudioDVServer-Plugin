#include "../include/audiocapture.h"


CADataCapture::CADataCapture()
{
	pEnumerator = NULL;
	pDevice = NULL;
	pAudioClient = NULL;
	pCaptureClient = NULL;
	pwfx = NULL;
	pData = NULL;
}

CADataCapture::~CADataCapture()
{
	CoTaskMemFree(pwfx);
	if (pEnumerator != NULL) pEnumerator->Release();
	if (pDevice != NULL) pDevice->Release();
	if (pAudioClient != NULL) pAudioClient->Release();
	if (pCaptureClient != NULL) pCaptureClient->Release();
	CoUninitialize();
}

HRESULT CADataCapture::Initial()
{
	HRESULT hr;
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to CoInitialize");
		return hr;
	}

	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to CoCreateInstance");
		return hr;
	}

	// get default output audio endpoint
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to GetDefaultAudioEndpoint");
		return hr;
	}

	// activates device
	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to Activate Decive");
		return hr;
	}

	// gets audio format
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to GetMixFormat");
		return hr;
	}

	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, pwfx, NULL);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to Initialize Audio Client");
		return hr;
	}

	UINT32 bufferFrameCount;
	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to GetBufferSize");
		return hr;
	}

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	if (FAILED(hr)) {
		LOG(ERR, L"Faild to GetService");
		return hr;
	}
	return S_OK;
}

HRESULT CADataCapture::Start()
{
	HRESULT hr;
	hr = pAudioClient->Start();
	
	if (FAILED(hr))
	{
		LOG(ERR, L"Failed to Start");
		return hr;
	}
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	return hr;
}

HRESULT CADataCapture::Stop()
{
	HRESULT hr;
	hr = pAudioClient->Stop();
	if (FAILED(hr))
	{
		LOG(ERR, L"Failed to Stop");
	}
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


