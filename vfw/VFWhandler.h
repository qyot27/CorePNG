#pragma once

#include <windows.h>
#include <vfw.h>
#include "ximage.h"

#define FOURCC_PNG mmioFOURCC('P','N','G','1')
#define FOURCC_PNG_OLD mmioFOURCC('P','N','G',' ')

class VFWhandler
{
public:

// Constructor and destructor

	VFWhandler(void);
	~VFWhandler(void);

// Config functions
	void VFW_configure();

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
	
protected:
	CxMemFile m_Memfile;	
	CxImage m_Image;
};
