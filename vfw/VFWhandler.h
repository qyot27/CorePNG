#pragma once

#include <windows.h>
#include <commctrl.h>
#include <vfw.h>
#include "ximage.h"

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

class VFWhandler
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
	inline RGBQUAD AveragePixels(DWORD x, DWORD y);
	
	DWORD m_BufferSize;
	CxImage myLogo;
	CxImage m_Image;
	CxImage m_DeltaFrame;

	bool m_DecodeToRGB24;
	DWORD m_DropFrameThreshold;
	WORD m_DeltaFrameCount;
	bool m_DeltaFramesEnabled;
};

BOOL CALLBACK ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

DWORD CorePNG_GetRegistryValue(char *value_key, DWORD default_value);
void CorePNG_SetRegistryValue(char *value_key, DWORD the_value);
