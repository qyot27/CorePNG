// ----------------------------------------------------------------------------
// CorePNG VFW Codec
// http://corecodec.org/projects/corepng
//
// Copyright (C) 2003 Jory Stone
// based on WARP VFW interface by General Lee D. Mented (gldm@mail.com)
//
// This file may be distributed under the terms of the Q Public License
// as defined by Trolltech AS of Norway and appearing in the file
// copying.txt included in the packaging of this file.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------

#ifndef __VFW_HANDLER_H_
#define __VFW_HANDLER_H_

#ifdef _DEBUG
//Memory Debugging includes
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <commctrl.h>
#include <vfw.h>
#include <assert.h>
#include <process.h>
#include <sstream>
#include <exception>
// CxImage includes
#undef _USRDLL 
// we don't want to export the CxImage API
#include "ximage.h"
#include "ximapng.h"
#define _USRDLL

// Colorspace conversion declares/includes
#include "convert_yuy2.h"
#include "convert_yv12.h"
extern "C" {
//Declare for assembly YUV2->RGB32 conversion
void _stdcall YUV422toRGB_MMX(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch);
void _stdcall YUV422toRGB24_MMX(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch);
//I don't use these, but it's nice to have the declares
void _stdcall RGBtoYCrCb_SSE2(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch); 
void _stdcall RGBtoYUV422_SSE2(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch); 
};

//#ifdef QUADRANT_SCAN_TEST
#include "quadrantscan.h"
//#endif

#include "cpu_info.h"

// .rc Resource include
#include "resource.h"

#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

#ifdef _DEBUG
#define ODS(x) OutputDebugString(x "\n")
#else
#define ODS(x)
#endif

// Debug builds have inlining disabled
#ifdef _DEBUG
#define INLINE
#else
#define INLINE inline
#endif

#define COREPNG_VFW_VERSION "CorePNG VFW Codec v0.8.1"
#define COREPNG_VFW_VERSIONW L"CorePNG VFW Codec v0.8.1"

#ifndef _DEBUG
#define VFW_CODEC_CRASH_CATCHER_START \
	try {
#define VFW_CODEC_CRASH_CATCHER_END \
	} catch (std::exception &ex) { \
		if (CorePNG_GetRegistryValue("Crash Catcher Enabled", 0)) { \
			std::ostringstream buf; \
			buf << "Exception: "; \
			buf << ex.what(); \
			buf << ("\nFile: " __FILE__); \
			/*buf << ("\nFunction: " __FUNCTION__) \*/ \
			buf << ("\nLine:"); \
			buf << __LINE__; \
			buf << "\nTry to generate bug report?"; \
			int msgBoxRet = MessageBoxA(NULL, buf.str().c_str(), "CorePNG VFW Codec Error", MB_YESNO|MB_TASKMODAL); \
			if (msgBoxRet == IDYES) { \
					throw; \
			} \
		} \
		return ICERR_ERROR; \
	} catch (...) { \
		if (CorePNG_GetRegistryValue("Crash Catcher Enabled", 0)) { \
			std::ostringstream buf; \
			buf << ("Unhandled Exception,"); \
			buf << ("\nFile: " __FILE__); \
			/*buf << ("\nFunction: " __FUNCTION__) \*/ \
			buf << ("\nLine:"); \
			buf << __LINE__; \
			buf << "\nTry to generate bug report?"; \
			int msgBoxRet = MessageBoxA(NULL, buf.str().c_str(), "CorePNG VFW Codec Error", MB_YESNO|MB_TASKMODAL); \
			if (msgBoxRet == IDYES) { \
					throw; \
			} \
		};\
		return ICERR_ERROR; \
	};
#endif

#ifndef VFW_CODEC_CRASH_CATCHER_START
#define VFW_CODEC_CRASH_CATCHER_START
#endif
#ifndef VFW_CODEC_CRASH_CATCHER_END
#define VFW_CODEC_CRASH_CATCHER_END
#endif

/*
#define VFW_CODEC_CRASH_CATCHER_END \
	} catch (...) {\
		return ICERR_ERROR;\
	};
*/
#define MAKE_FOURCC(x) mmioFOURCC(x[0], x[1], x[2], x[3])

#define FOURCC_YV12			MAKE_FOURCC("YV12")
#define FOURCC_IYUV			MAKE_FOURCC("IYUV")
#define FOURCC_YUY2			MAKE_FOURCC("YUY2")
#define FOURCC_UYVY			MAKE_FOURCC("UUYV")
#define FOURCC_YUYV			MAKE_FOURCC("YUYV")
#define FOURCC_YVYU			MAKE_FOURCC("YVYU")
#define FOURCC_PNG			MAKE_FOURCC("PNG1")
#define FOURCC_PNG_OLD	MAKE_FOURCC("PNG ")

/// Leave the output as orignal
#define PNGOutputMode_None  0x00
/// Convert output
#define PNGOutputMode_RGB24 0x01
#define PNGOutputMode_RGB32 0x02
#define PNGOutputMode_YUY2  0x03
#define PNGOutputMode_YVYU  0x04
#define PNGOutputMode_UYVY  0x05
#define PNGOutputMode_YV12  0x10
#define PNGOutputMode_IYUV  0x11

#define PNGFrameType_RGB24 0x01
#define PNGFrameType_YUY2  0x02
#define PNGFrameType_YV12  0x03

struct CorePNGCodecPrivate {
	CorePNGCodecPrivate() { 
		wSize = sizeof(CorePNGCodecPrivate); 
		bType = PNGFrameType_RGB24;
	};
	WORD wSize;
	BYTE bType;
};

struct CorePNGCodecSettings {
	CorePNGCodecSettings() {
		wSize = sizeof(CorePNGCodecSettings);
	}
	WORD wSize;
	BYTE m_ZlibCompressionLevel;
	BYTE m_PNGFilters;
	DWORD m_DropFrameThreshold;
	BYTE m_DeltaFramesEnabled;
	BYTE m_DeltaFrameAuto;
	WORD m_DeltaFrameLimit;
	BYTE m_EnableMultiThreading;
	BYTE m_EnableStatusDialog;
};

// wrapper for whatever critical section we have
class CCritSec {
public:
	CCritSec() {
		InitializeCriticalSection(&m_CritSec);
	};
	~CCritSec() {
		DeleteCriticalSection(&m_CritSec);
	};
	void Lock() {
		EnterCriticalSection(&m_CritSec);
	};
	void Unlock() {
		LeaveCriticalSection(&m_CritSec);
	};

protected:
	CRITICAL_SECTION m_CritSec;
private:
	// Make copy constructor and assignment operator inaccessible
	// Copying a critical section is a bad idea :P
	CCritSec(const CCritSec &refCritSec);
	CCritSec &operator=(const CCritSec &refCritSec);
};

// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class CAutoLock {
public:
	CAutoLock(CCritSec *plock) {
		m_pLock = plock;
		m_pLock->Lock();
	};
	~CAutoLock() {
		m_pLock->Unlock();
	};

protected:
	CCritSec *m_pLock;

private:
	// Make copy constructor and assignment operator inaccessible
	// Copying a AutoLock is a even worse idea :P
	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock);
};

class VFWhandler;

struct DeltaThreadInfo {	
	VFWhandler *handler;
	bool bRunning;	
	DWORD DeltaFrameSize;
	CRITICAL_SECTION csFrameData;
	//CRITICAL_SECTION csThreadBlock;
	ICCOMPRESS frameData;
};

class VFWhandler : public CorePNGCodecSettings
{
public:

// Constructor and destructor

	VFWhandler(void);
	~VFWhandler(void);

	void LoadSettings();
	void SaveSettings();
	
// Config functions
	void VFW_about(HWND hParentWindow);
	void VFW_configure(HWND hParentWindow);

// Compress functions

	int VFW_compress(ICCOMPRESS* lParam1, DWORD lParam2);
	int VFW_compress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_compress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_compress_get_size(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_compress_frames_info(ICCOMPRESSFRAMES* lParam1);
	int VFW_compress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_compress_end(int lParam1, int lParam2);

// Decompress functions

	int VFW_decompress(ICDECOMPRESS* lParam1, DWORD lParam2);
	int VFW_decompress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_decompress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_decompress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
	int VFW_decompress_end(int lParam1, int lParam2);

	// Dialogbox callbacks
	BOOL ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	BOOL AboutDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	BOOL StatusDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	
	INLINE int CompressDeltaFrame(ICCOMPRESS* lParam1, DWORD lParam2);
protected:
	INLINE void CopyImageBits(CxImage &dest, CxImage &src);
	static void __cdecl DeltaFrameCompressThread(void *threadData);
	INLINE bool CreateYUY2(BITMAPINFOHEADER* input);
	INLINE void CompressYUY2KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	INLINE void CompressYUY2DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	
	INLINE bool CreateYV12(BITMAPINFOHEADER* input);
	INLINE void CompressYV12KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	INLINE void CompressYV12DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);

	INLINE int CompressKeyFrame(ICCOMPRESS* lParam1, DWORD lParam2);	
	INLINE int CompressDeltaFrameAuto(ICCOMPRESS* lParam1, DWORD lParam2);
	INLINE RGBQUAD AveragePixels(DWORD x, DWORD y);
	
	/// Decompression functions
	INLINE int DecompressRGBFrame(ICDECOMPRESS* lParam1);
	INLINE int DecompressYUY2Frame(ICDECOMPRESS* lParam1);
	INLINE int DecompressYV12Frame(ICDECOMPRESS* lParam1);

	CorePNGCodecPrivate m_CodecPrivate;

	// Encoding mode
	DWORD m_Input_Mode;
	DWORD m_BufferSize;
	CxImage myLogo;
	CxImagePNG m_Image;
	CxImagePNG m_DeltaFrame;

	/// Memory buffer used for Auto-Delta frames
	CxMemFile m_MemoryBuffer;	
	/// The number of delta-frames since the last keyframe, 1-based. Ex. 1 means the last frame was a key frame
	WORD m_DeltaFrameCount;

	/// Decoding mode
	BYTE m_Output_Mode;
	bool m_DecodeToRGB24;
	/// Memory buffer used to hold decompressed frames when we have to convert to another format. Ex YUY2 -> RGB24
	CxMemFile m_DecodeBuffer;	

	DWORD m_Height;
	DWORD m_Width;
	CxImagePNG Y_Channel;		
	CxImagePNG U_Channel;		
	CxImagePNG V_Channel;
	CxImagePNG Y_Channel_Delta;		
	CxImagePNG U_Channel_Delta;		
	CxImagePNG V_Channel_Delta;
	DeltaThreadInfo *m_threadInfo;

	// Cpu infomation object
	CPUInfo m_cpu;

	// Handle to the status dialog
	HWND m_hwndStatusDialog;
	DWORD m_TotalKeyframes;
	DWORD m_TotalDeltaframes;
};

BOOL CALLBACK ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK StatusDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

WORD AddTooltip(HWND hwndTooltip, HWND hwndClient, LPSTR strText);

DWORD CorePNG_GetRegistryValue(char *value_key, DWORD default_value);
void CorePNG_SetRegistryValue(char *value_key, DWORD the_value);

#ifdef MY_OWN_COLORSPACE_CONVERSION
void YV12toRGB24(BYTE *pInput, BYTE *pOutput, DWORD dwWidth, DWORD dwHeight);
void YV12toYUY2(BYTE *pInput, BYTE *pOutput, DWORD dwWidth, DWORD dwHeight);
void YV12toRGB24_Resample(CxImage &Y_plane, CxImage &U_plane, CxImage &V_plane, BYTE *pOutput, WORD wMethod);
#endif

#endif // __VFW_HANDLER_H_

