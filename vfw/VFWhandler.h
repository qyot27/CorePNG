// ----------------------------------------------------------------------------
// CorePNG VFW Codec
// http://corecodec.org/projects/coreflac
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

#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif


#include <windows.h>
#include <commctrl.h>
#include <vfw.h>
#include <assert.h>
#include <process.h>
#include "ximage.h"
#include "ximapng.h"

extern "C" {
//Declare for assembly YUV2->RGB32 conversion
void _stdcall YUV422toRGB_MMX(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch);
//I don't use these, but it's nice to have the declares
void _stdcall RGBtoYCrCb_SSE2(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch); 
void _stdcall RGBtoYUV422_SSE2(void* lpIn,void* lpOut,DWORD dwFlags,DWORD dwWidth,DWORD dwHeight,DWORD dwSPitch,DWORD dwDPitch); 
};

#include "resource.h"

#define COREPNG_VFW_VERSION "CorePNG VFW Codec v0.7"
#define COREPNG_VFW_VERSIONW L"CorePNG VFW Codec v0.7"

#ifndef _DEBUG
#define VFW_CODEC_CRASH_CATCHER_START \
	try {
#define VFW_CODEC_CRASH_CATCHER_END \
	} catch (...) {\
		return ICERR_ERROR;\
	};
#else
#define VFW_CODEC_CRASH_CATCHER_START
#define VFW_CODEC_CRASH_CATCHER_END
#endif

#define FOURCC_YV12 mmioFOURCC('Y','V','1','2')
#define FOURCC_YUY2 mmioFOURCC('Y','U','Y','2')
#define FOURCC_PNG mmioFOURCC('P','N','G','1')
#define FOURCC_PNG_OLD mmioFOURCC('P','N','G',' ')

/// Leave the output as orignal
#define PNGOutputMode_None  0x00
/// Convert output
#define PNGOutputMode_RGB24 0x01
#define PNGOutputMode_RGB32 0x02
#define PNGOutputMode_YUY2  0x03
#define PNGOutputMode_YV12  0x04

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
	
	inline int CompressDeltaFrame(ICCOMPRESS* lParam1, DWORD lParam2);
protected:
	static void DeltaFrameCompressThread(void *threadData);
	inline bool CreateYUY2(BITMAPINFOHEADER* input);
	inline void CompressYUY2KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	inline void CompressYUY2DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	
	inline bool CreateYV12(BITMAPINFOHEADER* input);
	inline void CompressYV12KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);
	inline void CompressYV12DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer);

	inline int CompressKeyFrame(ICCOMPRESS* lParam1, DWORD lParam2);	
	inline int CompressDeltaFrameAuto(ICCOMPRESS* lParam1, DWORD lParam2);
	inline RGBQUAD AveragePixels(DWORD x, DWORD y);
	
	/// Decompression functions
	inline int DecompressRGBFrame(ICDECOMPRESS* lParam1);
	inline int DecompressYUY2Frame(ICDECOMPRESS* lParam1);
	inline int DecompressYV12Frame(ICDECOMPRESS* lParam1);

	CorePNGCodecPrivate m_CodecPrivate;

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
	DWORD m_HeightDouble;
	DWORD m_Width;
	CxImagePNG Y_Channel;		
	CxImagePNG U_Channel;		
	CxImagePNG V_Channel;
	CxImagePNG Y_Channel_Delta;		
	CxImagePNG U_Channel_Delta;		
	CxImagePNG V_Channel_Delta;
	DeltaThreadInfo *m_threadInfo;
};

BOOL CALLBACK ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

WORD AddTooltip(HWND hwndTooltip, HWND hwndClient, LPSTR strText);

DWORD CorePNG_GetRegistryValue(char *value_key, DWORD default_value);
void CorePNG_SetRegistryValue(char *value_key, DWORD the_value);

#endif // __VFW_HANDLER_H_

