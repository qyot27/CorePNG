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

#ifndef _COREPNG_H_
#define _COREPNG_H_

//Memory Leak Debuging define
#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <streams.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <queue>

#include "ximage.h"

#define ODS(x) OutputDebugString(x)

HRESULT DShowIMediaSampleCopy(IMediaSample *pSource, IMediaSample *pDest, bool bCopyData);

class CCorePNGEncoderFilter : public CTransformFilter {
public:	
	CCorePNGEncoderFilter(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCorePNGEncoderFilter();
	
	virtual HRESULT CheckInputType(const CMediaType *mtIn);
	virtual HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);	

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);

protected:			
	CxImage m_Image;
	CxMemFile memfile;	
	VIDEOINFOHEADER m_VideoHeader;
	long m_BufferSize;
};

class CCorePNGDecoderFilter : public CTransformFilter {
public:
	CCorePNGDecoderFilter(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCorePNGDecoderFilter();

	virtual HRESULT CheckInputType(const CMediaType *mtIn);
	virtual HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);

protected:
	CxImage m_Image;
	VIDEOINFOHEADER m_VideoHeader;
	long m_BufferSize;
	GUID m_OutputSubType;
};

class CCorePNGSubtitlerFilterPNGInputPin : public CTransformInputPin {
public:
	CCorePNGSubtitlerFilterPNGInputPin(TCHAR *pObjectName, CTransformFilter *pTransformFilter, HRESULT * phr, LPCWSTR pName);
	virtual HRESULT CheckMediaType(const CMediaType* pmt);
	virtual STDMETHODIMP Receive(IMediaSample *pSample);

protected:	

};

class CCorePNGSubtitlerFilterOutputPin : public CTransformOutputPin {
public:
	HRESULT Receive(IMediaSample *pSampleToPass) { 
		return this->m_pInputPin->Receive(pSampleToPass);
	};
};

class CCorePNGSubtitlerFilter : public CTransformFilter {
public:	
	CCorePNGSubtitlerFilter(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCorePNGSubtitlerFilter();
	
	virtual int GetPinCount() { return 3; };
	virtual CBasePin *GetPin(int n);
	virtual HRESULT CheckInputType(const CMediaType *mtIn);
	virtual HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);	
	HRESULT GetPNGSample(IMediaSample *pSample);
	HRESULT SetPNGHeader(VIDEOINFOHEADER *pVideoHeader);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);	

protected:
	HRESULT AlphaBlend(RGBTRIPLE *targetBits);
	inline HRESULT SubtitleReady(REFERENCE_TIME *pCurrentTimecode) { 
		if (*pCurrentTimecode >= m_LastTimecode)
			return S_OK; 
		return E_PENDING;
	};

	CCorePNGSubtitlerFilterPNGInputPin *m_pInputPNGPin;
	REFERENCE_TIME m_LastTimecode;
	CxImage m_Image;
	VIDEOINFOHEADER m_VideoHeader;
	VIDEOINFOHEADER m_PNGVideoHeader;
	long m_BufferSize;
};

#endif //_COREPNG_H_
