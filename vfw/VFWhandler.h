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

#include <windows.h>
#include <commctrl.h>
#include <vfw.h>
#include "ximage.h"
#include "ximapng.h"

#include "resource.h"

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

#define FOURCC_YUV2 mmioFOURCC('Y','U','V','2')
#define FOURCC_YUY2 mmioFOURCC('Y','U','Y','2')
#define FOURCC_PNG mmioFOURCC('P','N','G','1')
#define FOURCC_PNG_OLD mmioFOURCC('P','N','G',' ')

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

	BOOL ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
	inline int CompresssKeyFrame(ICCOMPRESS* lParam1, DWORD lParam2);
	inline int CompresssDeltaFrame(ICCOMPRESS* lParam1, DWORD lParam2);
	inline int CompresssDeltaFrameAuto(ICCOMPRESS* lParam1, DWORD lParam2);
	inline RGBQUAD AveragePixels(DWORD x, DWORD y);
	
	DWORD m_BufferSize;
	CxImage myLogo;
	CxImagePNG m_Image;
	CxImagePNG m_DeltaFrame;

	CxMemFile m_MemoryBuffer;
	//BYTE m_ZlibCompressionLevel;
	//BYTE m_PNGFilters;
	bool m_DecodeToRGB24;
	//DWORD m_DropFrameThreshold;
	WORD m_DeltaFrameCount;	
	//bool m_DeltaFramesEnabled;
};

BOOL CALLBACK ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

DWORD CorePNG_GetRegistryValue(char *value_key, DWORD default_value);
void CorePNG_SetRegistryValue(char *value_key, DWORD the_value);

#endif // __VFW_HANDLER_H_

