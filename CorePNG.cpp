// ----------------------------------------------------------------------------
// CorePNG
// http://corecodec.org/projects/coreflac
//
// Copyright (C) 2003 Jory Stone
//
// This file may be distributed under the terms of the Q Public License
// as defined by Trolltech AS of Norway and appearing in the file
// copying.txt included in the packaging of this file.
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

static wchar_t *g_swCorePNGSubtitlerName = L"CorePNG Video Subtitler";
static TCHAR *g_stCorePNGSubtitlerName = TEXT("CorePNG Video Subtitler");

// {228A3434-08CB-4602-8922-FF404B5FA126}
static const GUID CLSID_CorePNGEncoder = 
{ 0x228a3434, 0x8cb, 0x4602, { 0x89, 0x22, 0xff, 0x40, 0x4b, 0x5f, 0xa1, 0x26 } };

// {F5B73A39-DD6B-48c1-BBA5-9F95DE9A1029}
static const GUID CLSID_CorePNGDecoder =
{ 0xf5b73a39, 0xdd6b, 0x48c1, { 0xbb, 0xa5, 0x9f, 0x95, 0xde, 0x9a, 0x10, 0x29 } };

// {05B84A39-DD6B-48c1-BBA5-9F95DE9A1029}
static const GUID CLSID_CorePNGSubtitler =
{ 0x05b84a39, 0xdd6b, 0x48c1, { 0xbb, 0xa5, 0x9f, 0x95, 0xde, 0x9a, 0x10, 0x29 } };

#define FOURCC_PNG MAKEFOURCC('P', 'N', 'G', '1')
#define FOURCC_PNG_OLD MAKEFOURCC('P', 'N', 'G', ' ')

const static GUID MEDIASUBTYPE_PNG = (GUID)FOURCCMap(FOURCC_PNG);
const static GUID MEDIASUBTYPE_PNG_OLD = (GUID)FOURCCMap(FOURCC_PNG_OLD);

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
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PNG},
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PNG_OLD}
};

AMOVIESETUP_MEDIATYPE sudOutputType[] =
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 }
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

const AMOVIESETUP_FILTER sudCorePNGSubtitler =
{
    &CLSID_CorePNGSubtitler,   // clsID
    g_swCorePNGSubtitlerName,    // strName
    MERIT_DO_NOT_USE,         // dwMerit
    0,            // nPins
    0                   // lpPin
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
   {
		g_swCorePNGSubtitlerName,
		&CLSID_CorePNGSubtitler,
		CCorePNGSubtitlerFilter::CreateInstance,
		NULL, 
		&sudCorePNGSubtitler
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

/// Make destination an identical copy of source
HRESULT DShowIMediaSampleCopy(IMediaSample *pSource, IMediaSample *pDest, bool bCopyData)
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
		// We need RGB
		if (*mtIn->Subtype() == MEDIASUBTYPE_RGB24 || *mtIn->Subtype() == MEDIASUBTYPE_RGB32) {
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
	DShowIMediaSampleCopy(pIn, pOut, false);

	LONG lActual = m_VideoHeader.bmiHeader.biSizeImage;//pIn->GetActualDataLength();

	BYTE *pBuffer;
	pIn->GetPointer(&pBuffer);	

  bool bDropFrames = false;
  if (bDropFrames) {
  	// Check if this frame is the same as the last one
  	if (!memcmp(m_Image.GetBits(), pBuffer, lActual))
  		return S_FALSE;
	}

	if (m_VideoHeader.bmiHeader.biBitCount == 32) {
		// Convert the 24-bit+Alpha to 32-bits
		m_Image.CreateFromARGB(m_VideoHeader.bmiHeader.biWidth, m_VideoHeader.bmiHeader.biHeight, (BYTE *)pBuffer);
		m_Image.Flip();
	} else if (m_VideoHeader.bmiHeader.biBitCount == 24) {
		CopyMemory(m_Image.GetBits(), pBuffer, lActual);
	}
	
	memfile.Seek(0, 0);		
	m_Image.Encode(&memfile, CXIMAGE_FORMAT_PNG);	
	//m_Image.Draw(GetDC(NULL));

	lActual = memfile.Tell();	
	BYTE *pOutBuffer;
	pOut->GetPointer(&pOutBuffer);

	ASSERT(m_BufferSize > lActual);
	CopyMemory(pOutBuffer, memfile.GetBuffer(), lActual);

	pOut->SetActualDataLength(lActual);

	return S_OK;
};

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
		if (*mtIn->Subtype() == MEDIASUBTYPE_PNG || *mtIn->Subtype() == MEDIASUBTYPE_PNG_OLD) {
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

	if (mtOut->majortype != MEDIATYPE_Video || (mtOut->subtype != MEDIASUBTYPE_RGB24 && mtOut->subtype != MEDIASUBTYPE_RGB32))
		return S_FALSE;

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
			ppropInputRequest->cbBuffer = m_VideoHeader.bmiHeader.biSizeImage;
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
	ASSERT(iPosition == 0 || iPosition == 1 || iPosition == 2);

	if (iPosition == 0) {
		CheckPointer(pMediaType, E_POINTER);

		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_RGB24);

		// Set PNG format type
		m_VideoHeader.bmiHeader.biCompression = 0;
		pMediaType->SetFormat((BYTE *)&m_VideoHeader, sizeof(VIDEOINFOHEADER));

		pMediaType->SetFormatType(&FORMAT_VideoInfo);
		return S_OK;
	} else if (iPosition == 1) {
		CheckPointer(pMediaType, E_POINTER);

		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_RGB32);

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
		m_OutputSubType = pmt->subtype;
	}

	return hr;
}

HRESULT CCorePNGDecoderFilter::Transform(IMediaSample *pIn, IMediaSample *pOut) {
	DShowIMediaSampleCopy(pIn, pOut, false);

	LONG lActual = pIn->GetActualDataLength();

	BYTE *pBuffer;
	pIn->GetPointer(&pBuffer);

	BYTE *pOutBuffer;
	pOut->GetPointer(&pOutBuffer);
	m_Image.Decode(pBuffer, lActual, CXIMAGE_FORMAT_PNG);
	//m_Image.Draw(GetDC(NULL));
	if (m_OutputSubType == MEDIASUBTYPE_RGB24) {
		CopyMemory(pOutBuffer, m_Image.GetBits(), m_VideoHeader.bmiHeader.biSizeImage);
	
	} else if (m_OutputSubType == MEDIASUBTYPE_RGB32) {
		// Convert the 24-bit+Alpha to 32-bits
		m_Image.Flip();
		RGBTRIPLE *decodedImage = (RGBTRIPLE *)m_Image.GetBits();	
		RGBQUAD *decodedOutput = (RGBQUAD *)pOutBuffer;

		for (DWORD height = 0; height < m_Image.GetHeight(); height++) {
			for (DWORD width = 0; width < m_Image.GetWidth(); width++) {
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbBlue = decodedImage[(m_Image.GetWidth()*height)+width].rgbtBlue;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbGreen = decodedImage[(m_Image.GetWidth()*height)+width].rgbtGreen;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbRed = decodedImage[(m_Image.GetWidth()*height)+width].rgbtRed;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbReserved = m_Image.AlphaGet(width, height);
			}
		}
	}

	pOut->SetActualDataLength(m_VideoHeader.bmiHeader.biSizeImage);

	return S_OK;
};

CUnknown *WINAPI CCorePNGDecoderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CCorePNGDecoderFilter(pUnk, phr);
}

CCorePNGSubtitlerFilterPNGInputPin::CCorePNGSubtitlerFilterPNGInputPin(TCHAR *pObjectName, CTransformFilter *pTransformFilter, HRESULT * phr, LPCWSTR pName)
    : CTransformInputPin(pObjectName, pTransformFilter, phr, pName)
{
    DbgLog((LOG_TRACE,2,TEXT("CCorePNGSubtitlerFilterPNGInputPin::CCorePNGSubtitlerFilterPNGInputPin")));
}

HRESULT CCorePNGSubtitlerFilterPNGInputPin::CheckMediaType(const CMediaType* pmt)
{
   // Check the input type
	if (*pmt->Type() == MEDIATYPE_Video) {
		// Ok it's audio...
		// We need PNG
		if (*pmt->Subtype() == MEDIASUBTYPE_PNG) {
			// Yay \o/
			if (*pmt->FormatType() == FORMAT_VideoInfo) {				
				//memcpy(&m_VideoHeader, mtIn->Format(), sizeof(VIDEOINFOHEADER));
				//m_Image.Create(m_VideoHeader.bmiHeader.biWidth, m_VideoHeader.bmiHeader.biHeight, 24);
				return ((CCorePNGSubtitlerFilter *)m_pTransformFilter)->SetPNGHeader((VIDEOINFOHEADER *)pmt->Format());
			}
		}
	}
	return S_FALSE;	
};

STDMETHODIMP CCorePNGSubtitlerFilterPNGInputPin::Receive(IMediaSample *pSample)
{
  HRESULT hr;
  //CAutoLock lck(&m_pTransformFilter->m_csReceive);
  ASSERT(pSample);

  // check all is well with the base class
  hr = CBaseInputPin::Receive(pSample);
  if (S_OK == hr) {
      hr = ((CCorePNGSubtitlerFilter*)m_pTransformFilter)->GetPNGSample(pSample);
  }
	return hr;
};

CCorePNGSubtitlerFilter::CCorePNGSubtitlerFilter(LPUNKNOWN pUnk, HRESULT *pHr)
	: CTransformFilter(g_stCorePNGSubtitlerName, pUnk, CLSID_CorePNGSubtitler)
{
	memset(&m_VideoHeader, 0, sizeof(VIDEOINFOHEADER));
	memset(&m_PNGVideoHeader, 0, sizeof(VIDEOINFOHEADER));
};

CCorePNGSubtitlerFilter::~CCorePNGSubtitlerFilter() {
	delete m_pInputPNGPin;
};

CBasePin *CCorePNGSubtitlerFilter::GetPin(int n)
{
	HRESULT hr = S_OK;

	// Create an input pin if necessary

	if (m_pInput == NULL) {

		m_pInput = new CTransformInputPin(NAME("Transform input pin"),
			this,              // Owner filter
			&hr,               // Result code
			L"XForm In");      // Pin name


		//  Can't fail
		ASSERT(SUCCEEDED(hr));
		if (m_pInput == NULL) {
			return NULL;
		}

		m_pInputPNGPin = (CCorePNGSubtitlerFilterPNGInputPin *)
			new CCorePNGSubtitlerFilterPNGInputPin(NAME("PNG Subtitle input pin"),
			this,            // Owner filter
			&hr,             // Result code
			L"PNG Subtitle Stream");   // Pin name
		
		//  Can't fail
		ASSERT(SUCCEEDED(hr));
		if (m_pInputPNGPin == NULL) {
			return NULL;
		}

		m_pOutput = (CTransformOutputPin *)
			new CTransformOutputPin(NAME("Transform output pin"),
			this,            // Owner filter
			&hr,             // Result code
			L"XForm Out");   // Pin name

		// Can't fail
		ASSERT(SUCCEEDED(hr));
		if (m_pOutput == NULL) {
			delete m_pInput;
			m_pInput = NULL;
		}
	}

	// Return the appropriate pin

	if (n == 0) {
		return m_pInput;
	} else
		if (n == 1) {
			return m_pInputPNGPin;
		} else {
			if (n == 2) {
				return m_pOutput;
			} else {
				return NULL;
			}
		}
};

HRESULT CCorePNGSubtitlerFilter::CheckInputType(const CMediaType *mtIn) {
	if (*mtIn->Type() == MEDIATYPE_Video) {
		// Ok it's audio...
		// We need RAW video
		if (*mtIn->Subtype() == MEDIASUBTYPE_RGB24) {
			// Yay \o/
			memcpy(&m_VideoHeader, mtIn->Format(), sizeof(VIDEOINFOHEADER));
			m_Image.Create(m_VideoHeader.bmiHeader.biWidth, m_VideoHeader.bmiHeader.biHeight, 24);
			return S_OK;
		}
	}
	return S_FALSE;	
};

HRESULT CCorePNGSubtitlerFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {
	HRESULT hr = CheckInputType(mtIn);

	if (mtOut->majortype != MEDIATYPE_Video || mtOut->subtype != MEDIASUBTYPE_RGB24)
		return S_FALSE;

	if(hr == S_FALSE) {
		return hr;
	}

	return NOERROR;
};

HRESULT CCorePNGSubtitlerFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest) {
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

HRESULT CCorePNGSubtitlerFilter::GetMediaType(int iPosition, CMediaType *pMediaType) {
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

HRESULT CCorePNGSubtitlerFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt) {
	HRESULT hr = CTransformFilter::SetMediaType(direction, pmt);

	if (direction == PINDIR_INPUT) {

	} else if (direction == PINDIR_OUTPUT) {
		
	}

	return hr;
}

HRESULT CCorePNGSubtitlerFilter::Transform(IMediaSample *pIn, IMediaSample *pOut) {
	DShowIMediaSampleCopy(pIn, pOut, false);

	LONG lActual = pIn->GetActualDataLength();

	BYTE *pBuffer;
	pIn->GetPointer(&pBuffer);	

	//LONGLONG rtStart;
	//LONGLONG rtEnd;
	//pIn->GetMediaTime(&rtStart, &rtEnd);

	BYTE *pOutBuffer;
	pOut->GetPointer(&pOutBuffer);	
	if (m_HaveSubtitle) {
		BlendImages((RGBTRIPLE *)pBuffer, (RGBTRIPLE *)pOutBuffer);
		m_HaveSubtitle = false;
	} else {	
		//m_Image.Draw(GetDC(NULL));
		CopyMemory(pOutBuffer, pBuffer, m_VideoHeader.bmiHeader.biSizeImage);
	}
	pOut->SetActualDataLength(m_VideoHeader.bmiHeader.biSizeImage);

	return S_OK;
};

HRESULT CCorePNGSubtitlerFilter::GetPNGSample(IMediaSample *pSample)
{
  /*  Check for other streams and pass them on */
  AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
  if (pProps->dwStreamId != AM_STREAM_MEDIA) {
      return ((CCorePNGSubtitlerFilterOutputPin *)m_pOutput)->Receive(pSample);
  }
  HRESULT hr = S_OK;
  ASSERT(pSample);

	//LONGLONG rtEnd;
	//pSample->GetMediaTime(&m_LastTimecode, &rtEnd);
	// Decode the sample here
	LONG lActual = pSample->GetActualDataLength();

	BYTE *pBuffer;
	pSample->GetPointer(&pBuffer);

	m_Image.Decode(pBuffer, lActual, CXIMAGE_FORMAT_PNG);
	m_HaveSubtitle = true;

	return S_OK;
};

HRESULT CCorePNGSubtitlerFilter::SetPNGHeader(VIDEOINFOHEADER *pVideoHeader)
{
	memcpy(&m_PNGVideoHeader, pVideoHeader, sizeof(VIDEOINFOHEADER));
	m_Image.Create(m_PNGVideoHeader.bmiHeader.biWidth, m_PNGVideoHeader.bmiHeader.biHeight, 24); 
	return S_OK; 
};

HRESULT CCorePNGSubtitlerFilter::AlphaBlend(RGBTRIPLE *targetBits)
{
	/*CxImage dummySource(m_Image);
	RGBTRIPLE *sourcePNG = (RGBTRIPLE *)m_Image.GetBits();	
	RGBTRIPLE *sourceImage = (RGBTRIPLE *)dummySource.GetBits();

	for (DWORD height = 0; height < ; height++) {
		for (DWORD width = 0; width < m_Image.GetWidth(); width++) {
			BYTE sourcePNGAlpha = m_Image.AlphaGet(width, height);
				if (sourcePNGAlpha == 0) {
					// Fully transparent
					targetBits[(m_Image.GetWidth()*height)+width] = sourceImage[(m_Image.GetWidth()*height)+width];
				} else if (sourcePNGAlpha == 255) {
					// Fully solid
					targetBits[(m_Image.GetWidth()*height)+width] = sourcePNG[(m_Image.GetWidth()*height)+width];
				} else {
					// We have a blend
					RGBTRIPLE sourcePNGPixel = sourcePNG[(m_Image.GetWidth()*height)+width];
					RGBTRIPLE sourceImagePixel = sourceImage[(m_Image.GetWidth()*height)+width];
					RGBTRIPLE* targetBitsPixel = &targetBits[(m_Image.GetWidth()*height)+width];
					//dest' = ((weighting * source) + ((255-weighting) * dest)) / 256
					targetBits->rgbtBlue = sourceImagePixel.rgbtBlue * ((float)sourcePNGAlpha / 255) + sourcePNGPixel.rgbtBlue;
					targetBits->rgbtGreen = sourceImagePixel.rgbtGreen * ((float)sourcePNGAlpha / 255) + sourcePNGPixel.rgbtGreen;
					targetBits->rgbtRed = sourceImagePixel.rgbtRed * ((float)sourcePNGAlpha / 255) + sourcePNGPixel.rgbtRed;
				}
		}
	}*/
	return NOERROR;
};

BOOL CCorePNGSubtitlerFilter::BlendImages(RGBTRIPLE *lprgbSrc2, RGBTRIPLE *lprgbDst)
{
	RGBTRIPLE* targetBits = lprgbDst;
	RGBTRIPLE* sourcePNG = (RGBTRIPLE *)m_Image.GetBits();	
	RGBTRIPLE* sourceImage = (RGBTRIPLE *)lprgbSrc2;

	for (DWORD height = 0; height < m_Image.GetHeight(); height++) {
		for (DWORD width = 0; width < m_Image.GetWidth(); width++) {
			BYTE sourcePNGAlpha;//  m_Image.AlphaGet(width, height);
			
			RGBQUAD sourcePixel = m_Image.GetPixelColor(width, height);			
			if (sourcePixel.rgbBlue == 0 && sourcePixel.rgbGreen == 0 && sourcePixel.rgbRed == 0)
				sourcePNGAlpha = 0;

			if (sourcePNGAlpha == 0) {
				// Fully transparent
				targetBits[(m_Image.GetWidth()*height)+width] = sourceImage[(m_Image.GetWidth()*height)+width];
			} else if (sourcePNGAlpha == 255) {
				// Fully solid
				targetBits[(m_Image.GetWidth()*height)+width] = sourcePNG[(m_Image.GetWidth()*height)+width];
			} else {
				// We have a blend
				RGBTRIPLE sourcePNGPixel = sourcePNG[(m_Image.GetWidth()*height)+width];
				RGBTRIPLE sourceImagePixel = sourceImage[(m_Image.GetWidth()*height)+width];
				RGBTRIPLE* targetBitsPixel = &targetBits[(m_Image.GetWidth()*height)+width];
				//dest' = ((weighting * source) + ((255-weighting) * dest)) / 256
				targetBits->rgbtBlue = (sourceImagePixel.rgbtBlue * sourcePNGAlpha) + (sourcePNGPixel.rgbtBlue * (sourcePNGAlpha - 255));
				targetBits->rgbtGreen = (sourceImagePixel.rgbtGreen * sourcePNGAlpha) + (sourcePNGPixel.rgbtGreen * (sourcePNGAlpha - 255));
				targetBits->rgbtRed = (sourceImagePixel.rgbtRed * sourcePNGAlpha ) + (sourcePNGPixel.rgbtRed * (sourcePNGAlpha - 255));
			}
		}
	}

	/*DWORD dwWeight1;
	DWORD dwWeight2;
	DWORD dwWidthBytes = 1890;//m_VideoHeader.bmiHeader.biWidth * (m_VideoHeader.bmiHeader.biBitCount / 8);

	// Initialize the surface pointers.
	RGBTRIPLE* lprgbSrc1 = (RGBTRIPLE *)m_Image.GetBits();	
	//RGBTRIPLE* lprgbSrc2 = (RGBTRIPLE *)dummySource.GetBits();

	for (DWORD y = 0; y < m_Image.GetHeight(); y++) {
		for (DWORD x = 0; x < m_Image.GetWidth(); x++) {
			// Get this pixels alpha
			dwWeight1 = m_Image.AlphaGet(x, y);
			dwWeight2 = 255-dwWeight1;

			lprgbDst[x].rgbtRed   = (BYTE)((((DWORD)lprgbSrc1[x].rgbtRed * dwWeight1) + 
				((DWORD)lprgbSrc2[x].rgbtRed * dwWeight2)) >> 8);
			lprgbDst[x].rgbtGreen = (BYTE)((((DWORD)lprgbSrc1[x].rgbtGreen * dwWeight1) + 
				((DWORD)lprgbSrc2[x].rgbtGreen * dwWeight2)) >> 8);
			lprgbDst[x].rgbtBlue  = (BYTE)((((DWORD)lprgbSrc1[x].rgbtBlue * dwWeight1) + 
				((DWORD)lprgbSrc2[x].rgbtBlue * dwWeight2)) >> 8);
		}

		// Move to next scan line.
		lprgbSrc1 = (RGBTRIPLE *)((LPBYTE)lprgbSrc1 + dwWidthBytes);
		lprgbSrc2 = (RGBTRIPLE *)((LPBYTE)lprgbSrc2 + dwWidthBytes);
		lprgbDst  = (RGBTRIPLE *)((LPBYTE)lprgbDst  + dwWidthBytes);
	}
*/
	return TRUE;
}

CUnknown *WINAPI CCorePNGSubtitlerFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CCorePNGSubtitlerFilter(pUnk, phr);
}
