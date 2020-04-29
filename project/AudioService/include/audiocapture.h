#pragma once

#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <Audioclient.h>
#include <mmdeviceapi.h>

//Test
#include "../include/debug.h"

#define REFTIMES_PER_SEC  10000000


class CADataCapture
{
public:
	CADataCapture();
	~CADataCapture();

	HRESULT Initial();
	HRESULT Start();
	HRESULT Stop();
	HRESULT get_NextPacketSize();
	HRESULT get_Buffer();
	HRESULT ReleaseBuffer();

	UINT32 numFramesAvailable;
	UINT32 packetLength;
	BYTE* pData;
	DWORD flags;
	
private:
	const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);

	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice;
	IAudioClient* pAudioClient;
	IAudioCaptureClient* pCaptureClient;
	WAVEFORMATEX* pwfx;
};

#endif // !AUDIOCAPTURE_H
