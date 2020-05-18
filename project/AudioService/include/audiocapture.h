#pragma once

#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <Audioclient.h>
#include <mmdeviceapi.h>

//Test
#include "debug.h"

#define REFTIMES_PER_SEC  10000000


class CADataCapture : public IMMNotificationClient
{
public:
	CADataCapture();
	~CADataCapture();

	HRESULT Initial();
	HRESULT ExInitial();
	HRESULT Start();
	HRESULT Stop();
	HRESULT get_NextPacketSize();
	HRESULT get_Buffer();
	HRESULT ReleaseBuffer();
	void WaitBegin();
	void WaitEnd();
	bool IsChanging();

	UINT32 numFramesAvailable;
	UINT32 packetLength;
	BYTE* pData;
	DWORD flags;

public:
	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef(void) override;
	ULONG STDMETHODCALLTYPE Release(void) override;

	// IMMNotificationClient
	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(_In_  LPCWSTR pwstrDeviceId, _In_  DWORD dwNewState) override;
	HRESULT STDMETHODCALLTYPE OnDeviceAdded(_In_  LPCWSTR pwstrDeviceId) override;
	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(_In_  LPCWSTR pwstrDeviceId) override;
	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(_In_  EDataFlow flow, _In_  ERole role, _In_  LPCWSTR pwstrDefaultDeviceId) override;
	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(_In_  LPCWSTR pwstrDeviceId, _In_  const PROPERTYKEY key) override;

private:
	const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);

	ULONG mReferenceCount = 0;

	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice;
	IAudioClient* pAudioClient;
	IAudioCaptureClient* pCaptureClient;
	WAVEFORMATEX* pwfx;

	ERole role_;

	//主应用处于开始状态时将start置为true
	bool start_;
	//主应用在使用pCaptureClient时将wait置为true
	bool wait_;
	//当默认设备改变重新设置的过程中changing为true
	bool changing_;
};

#endif // !AUDIOCAPTURE_H
