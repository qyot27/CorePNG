// ----------------------------------------------------------------------------
// CorePNG
//
// Copyright (C) 2003 Jory Stone
//
// This file may be distributed under the terms of the Q Public License
// as defined by Trolltech AS of Norway and appearing in the file
// LICENSE.QPL included in the packaging of this file.
//
// This file may be distributed and/or modified under the terms of the
// GNU General Public License version 2 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL included in the
// packaging of this file.
//
// Licensees holding an other license may use this file in accordance with 
// the Agreement provided with the Software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------
#include "CorePNG.h"

static wchar_t *g_swCorePNGEncoderName = L"CorePNG Video Encoder";
static TCHAR *g_stCorePNGEncoderName = TEXT("CorePNG Video Encoder");

static wchar_t *g_swCorePNGDecoderName = L"CorePNG Video Decoder";
static TCHAR *g_stCorePNGDecoderName = TEXT("CorePNG Video Decoder");

// {228A3434-08CB-4602-8922-FF404B5FA126}
static const GUID CLSID_CorePNGEncoder = 
{ 0x228a3434, 0x8cb, 0x4602, { 0x89, 0x22, 0xff, 0x40, 0x4b, 0x5f, 0xa1, 0x26 } };

// {F5B73A39-DD6B-48c1-BBA5-9F95DE9A1029}
static const GUID CLSID_CorePNGDecoder = 
{ 0xf5b73a39, 0xdd6b, 0x48c1, { 0xbb, 0xa5, 0x9f, 0x95, 0xde, 0x9a, 0x10, 0x29 } };

#define FOURCC_PNG MAKEFOURCC('P', 'N', 'G', ' ')

const static GUID MEDIASUBTYPE_PNG = (GUID)FOURCCMap(FOURCC_PNG);

const AMOVIESETUP_FILTER sudCorePNGEncoder =
{
    &CLSID_CorePNGEncoder,   // clsID
    g_swCorePNGEncoderName,    // strName
    MERIT_DO_NOT_USE,         // dwMerit
    0,                        // nPins
    0                         // lpPin
};

AMOVIESETUP_MEDIATYPE sudInputType[] =
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PNG}
};

AMOVIESETUP_MEDIATYPE sudOutputType[] =
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 }
};

AMOVIESETUP_PIN sudPins[] =
{
	{ L"Input",
		FALSE,							// bRendered
		FALSE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudInputType),			// Number of media types
		sudInputType
	},
	{ L"Output",
		FALSE,							// bRendered
		TRUE,							// bOutput
		FALSE,							// bZero
		FALSE,							// bMany
		&CLSID_NULL,					// clsConnectsToFilter
		NULL,							// ConnectsToPin
		NUMELMS(sudOutputType),			// Number of media types
		sudOutputType
	}
};

const AMOVIESETUP_FILTER sudCorePNGDecoder =
{
    &CLSID_CorePNGDecoder,   // clsID
    g_swCorePNGDecoderName,    // strName
    MERIT_NORMAL,         // dwMerit
    NUMELMS(sudPins),            // nPins
    sudPins                   // lpPin
};


// Global data
CFactoryTemplate g_Templates[]= {
   {
		g_swCorePNGEncoderName, 
		&CLSID_CorePNGEncoder, 
		CCorePNGEncoderFilter::CreateInstance, 
		NULL, 
		&sudCorePNGEncoder
	 },
   {
		g_swCorePNGDecoderName, 
		&CLSID_CorePNGDecoder, 
		CCorePNGDecoderFilter::CreateInstance, 
		NULL, 
		&sudCorePNGDecoder
	 },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

CCorePNGEncoderFilter::CCorePNGEncoderFilter(LPUNKNOWN pUnk, HRESULT *pHr)
	: CTransformFilter(g_stCorePNGEncoderName, pUnk, CLSID_CorePNGEncoder)
{
	memset(&m_VideoHeader, 0, sizeof(VIDEOINFOHEADER));
	memfile.Open();
};

CCorePNGEncoderFilter::~CCorePNGEncoderFilter() {

};

HRESULT CCorePNGEncoderFilter::CheckInputType(const CMediaType *mtIn) {
	if (*mtIn->Type() == MEDIATYPE_Video) {
		// Ok it's audio...
		// We need PCM
		if (*mtIn->Subtype() == MEDIASUBTYPE_RGB24) {
			// Yay \o/
			memcpy(&m_VideoHeader, mtIn->Format(), sizeof(VIDEOINFOHEADER));
			m_Image.Create(m_VideoHeader.bmiHeader.biWidth, m_VideoHeader.bmiHeader.biHeight, 24);
			return S_OK;
		}
	}
	return S_FALSE;	
};

HRESULT CCorePNGEncoderFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {
	HRESULT hr = CheckInputType(mtIn);

	if(hr == S_FALSE) {
		return hr;
	}

	return NOERROR;
};

HRESULT CCorePNGEncoderFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest) {
	HRESULT hr = NOERROR;

	// Is the input pin connected
	if(m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(ppropInputRequest, E_POINTER);

	ppropInputRequest->cBuffers = 1;
	ppropInputRequest->cbAlign  = 1;

	// Get input pin's allocator size and use that
	ALLOCATOR_PROPERTIES InProps;
	IMemAllocator *pInAlloc = NULL;

	hr = m_pInput->GetAllocator(&pInAlloc);
	if(SUCCEEDED(hr)) {
		hr = pInAlloc->GetProperties(&InProps);
		if(SUCCEEDED(hr)) {
			ppropInputRequest->cbBuffer = InProps.cbBuffer * 2;
			m_BufferSize = ppropInputRequest->cbBuffer;
		}
		pInAlloc->Release();
	}

	if(FAILED(hr))
		return hr;

	ASSERT(ppropInputRequest->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(ppropInputRequest, &Actual);
	if(FAILED(hr)) {
		return hr;
	}

	ASSERT(Actual.cBuffers == 1);

	if(ppropInputRequest->cBuffers > Actual.cBuffers ||
		ppropInputRequest->cbBuffer > Actual.cbBuffer)
	{
		return E_FAIL;
	}

	return NOERROR;

};

HRESULT CCorePNGEncoderFilter::GetMediaType(int iPosition, CMediaType *pMediaType) {
	ASSERT(iPosition == 0 || iPosition == 1);
	
	if (iPosition == 0) {
		CheckPointer(pMediaType, E_POINTER);

		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_PNG);

		// Set PNG format type
		m_VideoHeader.bmiHeader.biCompression = FOURCC_PNG;
		pMediaType->SetFormat((BYTE *)&m_VideoHeader, sizeof(VIDEOINFOHEADER));		
		
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
};

HRESULT CCorePNGEncoderFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt) {
	HRESULT hr = CTransformFilter::SetMediaType(direction, pmt);

	if (direction == PINDIR_INPUT) {

	} else if (direction == PINDIR_OUTPUT) {
		
	}

	return hr;
}

HRESULT CCorePNGEncoderFilter::Transform(IMediaSample *pIn, IMediaSample *pOut) {
	Copy(pIn, pOut, false);

	LONG lActual = m_VideoHeader.bmiHeader.biSizeImage;//pIn->GetActualDataLength();

	BYTE *pBuffer;
	pIn->GetPointer(&pBuffer);	

	CopyMemory(m_Image.GetBits(), pBuffer, lActual);
	memfile.Seek(0, 0);
	m_Image.Encode(&memfile, CXIMAGE_FORMAT_PNG);	
	//m_Image.Draw(GetDC(NULL));

	lActual = memfile.Size();	
	BYTE *pOutBuffer;
	pOut->GetPointer(&pOutBuffer);

	ASSERT(m_BufferSize > lActual);
	CopyMemory(pOutBuffer, memfile.GetBuffer(), lActual);

	pOut->SetActualDataLength(lActual);

	return S_OK;
};

/// Make destination an identical copy of source
HRESULT CCorePNGEncoderFilter::Copy(IMediaSample *pSource, IMediaSample *pDest, bool bCopyData) const
{
	CheckPointer(pSource, E_POINTER);
	CheckPointer(pDest, E_POINTER);

	if (bCopyData) {
		// Copy the sample data
		BYTE *pSourceBuffer, *pDestBuffer;
		long lSourceSize = pSource->GetActualDataLength();

	#ifdef DEBUG    
		long lDestSize = pDest->GetSize();
		ASSERT(lDestSize >= lSourceSize);
	#endif

		pSource->GetPointer(&pSourceBuffer);
		pDest->GetPointer(&pDestBuffer);

		CopyMemory((PVOID) pDestBuffer,(PVOID) pSourceBuffer, lSourceSize);
	}

	// Copy the sample times
	REFERENCE_TIME TimeStart, TimeEnd;
	if(NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
	{
		pDest->SetTime(&TimeStart, &TimeEnd);
	}

	LONGLONG MediaStart, MediaEnd;
	if(pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
	{
		pDest->SetMediaTime(&MediaStart,&MediaEnd);
	}

	// Copy the media type
	AM_MEDIA_TYPE *pMediaType;
	pSource->GetMediaType(&pMediaType);
	pDest->SetMediaType(pMediaType);
	DeleteMediaType(pMediaType);

	// Copy the actual data length
	long lDataLength = pSource->GetActualDataLength();
	pDest->SetActualDataLength(lDataLength);

	return NOERROR;
}

CUnknown *WINAPI CCorePNGEncoderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CCorePNGEncoderFilter(pUnk, phr);
}

CCorePNGDecoderFilter::CCorePNGDecoderFilter(LPUNKNOWN pUnk, HRESULT *pHr)
	: CTransformFilter(g_stCorePNGDecoderName, pUnk, CLSID_CorePNGDecoder)
{
	memset(&m_VideoHeader, 0, sizeof(VIDEOINFOHEADER));
};

CCorePNGDecoderFilter::~CCorePNGDecoderFilter() {

};

HRESULT CCorePNGDecoderFilter::CheckInputType(const CMediaType *mtIn) {
	if (*mtIn->Type() == MEDIATYPE_Video) {
		// Ok it's audio...
		// We need PCM
		if (*mtIn->Subtype() == MEDIASUBTYPE_PNG) {
			// Yay \o/
			memcpy(&m_VideoHeader, mtIn->Format(), sizeof(VIDEOINFOHEADER));
			m_Image.Create(m_VideoHeader.bmiHeader.biWidth, m_VideoHeader.bmiHeader.biHeight, 24);
			return S_OK;
		}
	}
	return S_FALSE;	
};

HRESULT CCorePNGDecoderFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {
	HRESULT hr = CheckInputType(mtIn);

	if(hr == S_FALSE) {
		return hr;
	}

	return NOERROR;
};

HRESULT CCorePNGDecoderFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest) {
	HRESULT hr = NOERROR;

	// Is the input pin connected
	if(m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(ppropInputRequest, E_POINTER);

	ppropInputRequest->cBuffers = 1;
	ppropInputRequest->cbAlign  = 1;

	// Get input pin's allocator size and use that
	ALLOCATOR_PROPERTIES InProps;
	IMemAllocator *pInAlloc = NULL;

	hr = m_pInput->GetAllocator(&pInAlloc);
	if(SUCCEEDED(hr)) {
		hr = pInAlloc->GetProperties(&InProps);
		if(SUCCEEDED(hr)) {
			ppropInputRequest->cbBuffer = InProps.cbBuffer * InProps.cBuffers;
			m_BufferSize = ppropInputRequest->cbBuffer;
		}
		pInAlloc->Release();
	}

	if(FAILED(hr))
		return hr;

	ASSERT(ppropInputRequest->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(ppropInputRequest, &Actual);
	if(FAILED(hr)) {
		return hr;
	}

	ASSERT(Actual.cBuffers == 1);

	if(ppropInputRequest->cBuffers > Actual.cBuffers ||
		ppropInputRequest->cbBuffer > Actual.cbBuffer)
	{
		return E_FAIL;
	}

	return NOERROR;

};

HRESULT CCorePNGDecoderFilter::GetMediaType(int iPosition, CMediaType *pMediaType) {
	ASSERT(iPosition == 0 || iPosition == 1);
	
	if (iPosition == 0) {
		CheckPointer(pMediaType, E_POINTER);

		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_RGB24);

		// Set PNG format type
		m_VideoHeader.bmiHeader.biCompression = 0;
		pMediaType->SetFormat((BYTE *)&m_VideoHeader, sizeof(VIDEOINFOHEADER));		
		
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
};

HRESULT CCorePNGDecoderFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt) {
	HRESULT hr = CTransformFilter::SetMediaType(direction, pmt);

	if (direction == PINDIR_INPUT) {

	} else if (direction == PINDIR_OUTPUT) {
		
	}

	return hr;
}

HRESULT CCorePNGDecoderFilter::Transform(IMediaSample *pIn, IMediaSample *pOut) {
	Copy(pIn, pOut, false);

	LONG lActual = pIn->GetActualDataLength();

	BYTE *pBuffer;
	pIn->GetPointer(&pBuffer);	

	BYTE *pOutBuffer;
	pOut->GetPointer(&pOutBuffer);	
	m_Image.Decode(pBuffer, lActual, CXIMAGE_FORMAT_PNG);
	//m_Image.Draw(GetDC(NULL));

	CopyMemory(pOutBuffer, m_Image.GetBits(), m_VideoHeader.bmiHeader.biSizeImage);

	pOut->SetActualDataLength(m_VideoHeader.bmiHeader.biSizeImage);

	return S_OK;
};

/// Make destination an identical copy of source
HRESULT CCorePNGDecoderFilter::Copy(IMediaSample *pSource, IMediaSample *pDest, bool bCopyData) const
{
	CheckPointer(pSource, E_POINTER);
	CheckPointer(pDest, E_POINTER);

	if (bCopyData) {
		// Copy the sample data
		BYTE *pSourceBuffer, *pDestBuffer;
		long lSourceSize = pSource->GetActualDataLength();

	#ifdef DEBUG    
		long lDestSize = pDest->GetSize();
		ASSERT(lDestSize >= lSourceSize);
	#endif

		pSource->GetPointer(&pSourceBuffer);
		pDest->GetPointer(&pDestBuffer);

		CopyMemory((PVOID) pDestBuffer,(PVOID) pSourceBuffer, lSourceSize);
	}

	// Copy the sample times
	REFERENCE_TIME TimeStart, TimeEnd;
	if(NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
	{
		pDest->SetTime(&TimeStart, &TimeEnd);
	}

	LONGLONG MediaStart, MediaEnd;
	if(pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
	{
		pDest->SetMediaTime(&MediaStart,&MediaEnd);
	}

	// Copy the media type
	AM_MEDIA_TYPE *pMediaType;
	pSource->GetMediaType(&pMediaType);
	pDest->SetMediaType(pMediaType);
	DeleteMediaType(pMediaType);

	// Copy the actual data length
	long lDataLength = pSource->GetActualDataLength();
	pDest->SetActualDataLength(lDataLength);

	return NOERROR;
}

CUnknown *WINAPI CCorePNGDecoderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CCorePNGDecoderFilter(pUnk, phr);
}
