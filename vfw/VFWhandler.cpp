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
/*
	About 768 byte of 256 palette overhead per image.

*/

#include "VFWhandler.h"

extern HINSTANCE g_hInst;

bool VFWhandler::CreateYUY2(BITMAPINFOHEADER* input)
{
	m_YUV_Mode = 0;

	m_ImageSize = input->biSizeImage;

	m_Height = input->biHeight;
	m_HeightDouble = m_Height * 2;
	m_Width = input->biWidth;
	
	Y_Channel.Create(m_Width, m_Height, 8);
	U_Channel.Create(m_Width/2, m_Height, 8);
	V_Channel.Create(m_Width/2, m_Height, 8);
	
	Y_Channel.GrayScale();
	U_Channel.GrayScale();
	V_Channel.GrayScale();
	//Y_Channel.SetGrayPalette();
	//U_Channel.SetGrayPalette();
	//V_Channel.SetGrayPalette();

	Y_Channel_Delta = Y_Channel;
	U_Channel_Delta = U_Channel;
	V_Channel_Delta = V_Channel;

	return true;
};

void VFWhandler::CompressYUY2KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer)
{
	// YUY2
	BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
	BYTE *U_data = (BYTE *)U_Channel.GetBits();
	BYTE *V_data = (BYTE *)V_Channel.GetBits();

	for (DWORD height = 0; height < m_HeightDouble; height++) {
		for (DWORD width = 0; width < m_Width; width += 4) {
			Y_data++[0] = inputYUV2Data[width+0];
			U_data++[0] = inputYUV2Data[width+1];
			Y_data++[0] = inputYUV2Data[width+2];
			V_data++[0] = inputYUV2Data[width+3];
		}
		inputYUV2Data += width;
	}

	if (m_DeltaFramesEnabled) {
		memcpy(Y_Channel_Delta.GetBits(), Y_Channel.GetBits(), m_Height*m_Width);
		memcpy(U_Channel_Delta.GetBits(), U_Channel.GetBits(), m_Height*m_Width/2);
		memcpy(V_Channel_Delta.GetBits(), V_Channel.GetBits(), m_Height*m_Width/2);
		m_DeltaFrameCount++;
	}
	//Y_Channel.Draw(GetDC(NULL));
	//U_Channel.Draw(GetDC(NULL));
	//V_Channel.Draw(GetDC(NULL));
	
	// Encode YUV2
	Y_Channel.Encode(targetBuffer);	
	U_Channel.Encode(targetBuffer);	
	V_Channel.Encode(targetBuffer);	
};

void VFWhandler::CompressYUY2DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer)
{
	// YUY2
	BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
	BYTE *U_data = (BYTE *)U_Channel.GetBits();
	BYTE *V_data = (BYTE *)V_Channel.GetBits();
	
	BYTE *Y_Delta_data = (BYTE *)Y_Channel_Delta.GetBits();
	BYTE *U_Delta_data = (BYTE *)U_Channel_Delta.GetBits();
	BYTE *V_Delta_data = (BYTE *)V_Channel_Delta.GetBits();
	
	// Copy and Compare to the previous frame
	for (DWORD height = 0; height < m_HeightDouble; height++) {
		for (DWORD width = 0; width < m_Width; width += 4) {
			Y_data++[0] = inputYUV2Data[width+0] - Y_Delta_data[0];
			Y_Delta_data++[0] = inputYUV2Data[width+0];

			U_data++[0] = inputYUV2Data[width+1] - U_Delta_data[0];
			U_Delta_data++[0] = inputYUV2Data[width+1];

			Y_data++[0] = inputYUV2Data[width+2] - Y_Delta_data[0];
			Y_Delta_data++[0] = inputYUV2Data[width+2];

			V_data++[0] = inputYUV2Data[width+3] - V_Delta_data[0];
			V_Delta_data++[0] = inputYUV2Data[width+3];
		}
		inputYUV2Data += width;
	}		

	if (m_DeltaFrameCount++ > m_DeltaFrameLimit)
		m_DeltaFrameCount = 0;

	//Y_Channel.Draw(GetDC(NULL));
	//U_Channel.Draw(GetDC(NULL));
	//V_Channel.Draw(GetDC(NULL));
	
	// Encode YUV2
	Y_Channel.Encode(targetBuffer);	
	U_Channel.Encode(targetBuffer);	
	V_Channel.Encode(targetBuffer);	
};

bool VFWhandler::CreateYV12(BITMAPINFOHEADER* input)
{
	m_YUV_Mode = 0;

	m_ImageSize = input->biSizeImage;

	m_Height = input->biHeight;
	m_HeightDouble = m_Height * 2;
	m_Width = input->biWidth;
	
	Y_Channel.Create(m_Width, m_Height, 8);
	U_Channel.Create(m_Width/2, m_Height/2, 8);
	V_Channel.Create(m_Width/2, m_Height/2, 8);

	Y_Channel.GrayScale();
	U_Channel.GrayScale();
	V_Channel.GrayScale();

	Y_Channel_Delta = Y_Channel;
	U_Channel_Delta = U_Channel;
	V_Channel_Delta = V_Channel;

	return true;
};

void VFWhandler::CompressYV12KeyFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer)
{
	// YV12
	BYTE *Y_data = (BYTE *)Y_Channel.GetBits();	
	BYTE *V_data = (BYTE *)V_Channel.GetBits();
	BYTE *U_data = (BYTE *)U_Channel.GetBits();

	memcpy(Y_Channel.GetBits(), inputYUV2Data, Y_Channel.GetSizeImage());
	inputYUV2Data += Y_Channel.GetSizeImage();
	memcpy(V_Channel.GetBits(), inputYUV2Data, V_Channel.GetSizeImage());
	inputYUV2Data += V_Channel.GetSizeImage();
	memcpy(U_Channel.GetBits(), inputYUV2Data, U_Channel.GetSizeImage());


	if (m_DeltaFramesEnabled) {
		memcpy(Y_Channel_Delta.GetBits(), Y_Channel.GetBits(), Y_Channel.GetSizeImage());
		memcpy(U_Channel_Delta.GetBits(), U_Channel.GetBits(), U_Channel.GetSizeImage());
		memcpy(V_Channel_Delta.GetBits(), V_Channel.GetBits(), V_Channel.GetSizeImage());
		m_DeltaFrameCount++;
	}
	//Y_Channel.Draw(GetDC(NULL));
	//U_Channel.Draw(GetDC(NULL));
	//V_Channel.Draw(GetDC(NULL));
	
	// Encode YV12
	Y_Channel.Encode(targetBuffer);	
	V_Channel.Encode(targetBuffer);	
	U_Channel.Encode(targetBuffer);	
};

void VFWhandler::CompressYV12DeltaFrame(BYTE *inputYUV2Data, CxMemFile *targetBuffer)
{
	// YV12
	BYTE* inputYUV2DataOld = inputYUV2Data;
	BYTE *Y_data = (BYTE *)Y_Channel.GetBits();	
	BYTE *V_data = (BYTE *)V_Channel.GetBits();
	BYTE *U_data = (BYTE *)U_Channel.GetBits();

	BYTE *Y_Delta_data = (BYTE *)Y_Channel_Delta.GetBits();
	BYTE *U_Delta_data = (BYTE *)U_Channel_Delta.GetBits();
	BYTE *V_Delta_data = (BYTE *)V_Channel_Delta.GetBits();

	for (DWORD d = 0; d < Y_Channel.GetSizeImage(); d++)
		Y_data++[0] = inputYUV2Data++[0] - Y_Delta_data++[0];
		
	for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
		V_data++[0] = inputYUV2Data++[0] - V_Delta_data++[0];

	for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
		U_data++[0] = inputYUV2Data++[0] - U_Delta_data++[0];

	inputYUV2Data = inputYUV2DataOld;
	memcpy(Y_Channel_Delta.GetBits(), inputYUV2Data, Y_Channel_Delta.GetSizeImage());
	inputYUV2Data += Y_Channel_Delta.GetSizeImage();
	memcpy(V_Channel_Delta.GetBits(), inputYUV2Data, V_Channel_Delta.GetSizeImage());
	inputYUV2Data += V_Channel_Delta.GetSizeImage();
	memcpy(U_Channel_Delta.GetBits(), inputYUV2Data, U_Channel_Delta.GetSizeImage());

	
	//memcpy(Y_Channel_Delta.GetBits(), Y_Channel.GetBits(), Y_Channel.GetSizeImage());
	//memcpy(U_Channel_Delta.GetBits(), U_Channel.GetBits(), U_Channel.GetSizeImage());
	//memcpy(V_Channel_Delta.GetBits(), V_Channel.GetBits(), V_Channel.GetSizeImage());

	if (m_DeltaFrameCount++ > m_DeltaFrameLimit)
		m_DeltaFrameCount = 0;

	//Y_Channel.Draw(GetDC(NULL));
	//U_Channel.Draw(GetDC(NULL));
	//V_Channel.Draw(GetDC(NULL));
	
	// Encode YV12
	Y_Channel.Encode(targetBuffer);	
	V_Channel.Encode(targetBuffer);	
	U_Channel.Encode(targetBuffer);		
};

VFWhandler::VFWhandler(void)
{
	m_BufferSize = 0;
	m_DeltaFrameCount = 0;
	LoadSettings();
	m_MemoryBuffer.Open();
}

VFWhandler::~VFWhandler(void)
{
	//SaveSettings();
}

void VFWhandler::LoadSettings() {	
	m_ZlibCompressionLevel = CorePNG_GetRegistryValue("zlib Compression Level", 6);
	m_PNGFilters = CorePNG_GetRegistryValue("PNG Filters", PNG_ALL_FILTERS);
	m_DeltaFramesEnabled = CorePNG_GetRegistryValue("Use Delta Frames", 0);
	m_DeltaFrameAuto = CorePNG_GetRegistryValue("Auto Delta Frames", 0);
	m_DeltaFrameLimit = CorePNG_GetRegistryValue("Keyframe Interval", 30);
	m_DropFrameThreshold = CorePNG_GetRegistryValue("Drop Frame Threshold", 0);
	m_DecodeToRGB24 = CorePNG_GetRegistryValue("Always Decode to RGB24", 0);
}

void VFWhandler::SaveSettings() {	
	CorePNG_SetRegistryValue("zlib Compression Level", m_ZlibCompressionLevel);
	CorePNG_SetRegistryValue("PNG Filters", m_PNGFilters);
	CorePNG_SetRegistryValue("Use Delta Frames", m_DeltaFramesEnabled);
	CorePNG_SetRegistryValue("Auto Delta Frames", m_DeltaFrameAuto);
	CorePNG_SetRegistryValue("Keyframe Interval", m_DeltaFrameLimit);	
	CorePNG_SetRegistryValue("Drop Frame Threshold", m_DropFrameThreshold);
	CorePNG_SetRegistryValue("Always Decode to RGB24", m_DecodeToRGB24);
}

/******************************************************************************
* VFW_Configure();
*
* This basicly pops up the dialog box to configure codec settings. It will
* store the settings in the WARPconfig myconfig (see below) and then
* pass them to the codec during VFW_compress_begin.
*
******************************************************************************/

void VFWhandler::VFW_configure(HWND hParentWindow)
{
	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_PROPPAGE_CONFIG), hParentWindow, ::ConfigurationDlgProc, (LPARAM)this);	
}

/******************************************************************************
* VFW_compress(ICCOMPRESS* lParam1, DWORD lParam2);
*
* This function is used to compress individual frames during compression.
*
******************************************************************************/

int VFWhandler::VFW_compress(ICCOMPRESS* lParam1, DWORD lParam2)
{
	VFW_CODEC_CRASH_CATCHER_START;
	
	if (!m_DeltaFrameCount) {
		// Now compress the frame.
		if (CompressKeyFrame(lParam1, lParam2) != ICERR_OK)
			return ICERR_ERROR;

	} else {
		// We compress a delta frame
		if (!m_DeltaFrameAuto) {
			// Every frame is a delta
			if (CompressDeltaFrame(lParam1, lParam2) != ICERR_OK)
				return ICERR_ERROR;
		} else {
			// Use an Auto setting
			if (CompressDeltaFrameAuto(lParam1, lParam2) != ICERR_OK)
				return ICERR_ERROR;
		}
	}
	return ICERR_OK;

	VFW_CODEC_CRASH_CATCHER_END;
}

int VFWhandler::CompressKeyFrame(ICCOMPRESS* lParam1, DWORD lParam2) 
{
	DWORD lActual = lParam1->lpbiInput->biSizeImage;
		
	if (lParam1->lpbiInput->biBitCount == 24) {
		if (m_DropFrameThreshold != 0) {
			DWORD imageDiff = abs(memcmp(m_Image.GetBits(), lParam1->lpInput, lActual));
			if (imageDiff < m_DropFrameThreshold) {
				// Drop this frame
				lActual = 0;
				// Drop a frame	
				lParam1->lpbiOutput->biSizeImage = 0;
				lParam1->dwFlags = 0;
				*lParam1->lpdwFlags = 0;
				return ICERR_OK;
			}
		}
		memcpy(m_Image.GetBits(), lParam1->lpInput, lActual);
		if (m_DeltaFramesEnabled) {
			m_DeltaFrame = m_Image;
			m_DeltaFrameCount++;
		}
		
		// Encode 24-bit
		if (lParam1->lpOutput != NULL) {
			// Write directly to the VFW buffer
			CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
			m_Image.Encode(&memFile);	
			//m_Image.Draw(GetDC(NULL));

			lActual = memFile.Tell();	
		} else {
			// Write to our tempory memory buffer instead
			m_MemoryBuffer.Seek(0, 0);
			m_Image.Encode(&m_MemoryBuffer);	
			//m_Image.Draw(GetDC(NULL));

			lActual = m_MemoryBuffer.Tell();	
		}
	} else if (lParam1->lpbiInput->biBitCount == 32) {
		m_Image.CreateFromARGB(lParam1->lpbiInput->biWidth, lParam1->lpbiInput->biHeight, (BYTE *)lParam1->lpInput);
		m_Image.Flip();

		// Encode 32-bit
		if (lParam1->lpOutput != NULL) {
			// Write directly to the VFW buffer
			CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
			m_Image.Encode(&memFile);	
			//m_Image.Draw(GetDC(NULL));

			lActual = memFile.Tell();	
		} else {
			// Write to our tempory memory buffer instead
			m_MemoryBuffer.Seek(0, 0);
			m_Image.Encode(&m_MemoryBuffer);	
			//m_Image.Draw(GetDC(NULL));

			lActual = m_MemoryBuffer.Tell();	
		}
	} else if (lParam1->lpbiInput->biCompression == FOURCC_YUY2 && lParam1->lpbiInput->biBitCount == 16) {
		if (lParam1->lpOutput != NULL) {
			// Write directly to the VFW buffer
			CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);			
			CompressYUY2KeyFrame((BYTE *)lParam1->lpInput, &memFile);	
			lActual = memFile.Tell();	
		} else {
			// Write to our tempory memory buffer instead
			m_MemoryBuffer.Seek(0, 0);			
			CompressYUY2KeyFrame((BYTE *)lParam1->lpInput, &m_MemoryBuffer);
			lActual = m_MemoryBuffer.Tell();	
		}			
	} else if (lParam1->lpbiInput->biCompression == FOURCC_YV12 && lParam1->lpbiInput->biBitCount == 12) {
		if (lParam1->lpOutput != NULL) {
			// Write directly to the VFW buffer
			CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);			
			CompressYV12KeyFrame((BYTE *)lParam1->lpInput, &memFile);	
			lActual = memFile.Tell();	
		} else {
			// Write to our tempory memory buffer instead
			m_MemoryBuffer.Seek(0, 0);			
			CompressYV12KeyFrame((BYTE *)lParam1->lpInput, &m_MemoryBuffer);
			lActual = m_MemoryBuffer.Tell();	
		}	
	}

	// Now put the result back	
	lParam1->lpbiOutput->biSizeImage = lActual;
	lParam1->dwFlags = ICCOMPRESS_KEYFRAME;
	*lParam1->lpdwFlags = AVIIF_KEYFRAME;	

	return ICERR_OK;
};

int VFWhandler::CompressDeltaFrame(ICCOMPRESS* lParam1, DWORD lParam2)
{
	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	if (lParam1->lpbiInput->biBitCount == 24) {
		if (m_DropFrameThreshold != 0) {
			DWORD imageDiff = abs(memcmp(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual));
			if (imageDiff < m_DropFrameThreshold) {
				// Drop this frame
				lParam1->lpbiOutput->biSizeImage = 0;
				lParam1->dwFlags = 0;
				*lParam1->lpdwFlags = 0;	
				// Increase the delta-frame count even though we drop this frame
				m_DeltaFrameCount++;

				return ICERR_OK;
			}
		}
		memcpy(m_Image.GetBits(), lParam1->lpInput, lActual);

		// Compare to the previous frame
		m_Image.Mix(m_DeltaFrame, CxImage::ImageOpType::OpSub2);

		// Replace the delta frame with the full frame
		memcpy(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual);
		if (m_DeltaFrameCount++ > m_DeltaFrameLimit)
			m_DeltaFrameCount = 0;

		// Actually encode the frame
		CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
		m_Image.Encode(&memFile);	
		//m_Image.Draw(GetDC(NULL));
		lActual = memFile.Tell();	

	} else if (lParam1->lpbiInput->biBitCount == 32) {
		if (m_DropFrameThreshold != 0) {
			DWORD imageDiff = abs(memcmp(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual));
			if (imageDiff < m_DropFrameThreshold) {
				// Drop this frame
				lParam1->lpbiOutput->biSizeImage = 0;
				lParam1->dwFlags = 0;
				*lParam1->lpdwFlags = 0;	
				// Increase the delta-frame count even though we drop this frame
				m_DeltaFrameCount++;

				return ICERR_OK;
			}
		}
		m_Image.CreateFromARGB(lParam1->lpbiInput->biWidth, lParam1->lpbiInput->biHeight, (BYTE *)lParam1->lpInput);

		// Compare to the previous frame
		m_Image.Mix(m_DeltaFrame, CxImage::ImageOpType::OpSub2);

		// Replace the delta frame with the full frame
		memcpy(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual);
		if (m_DeltaFrameCount++ > m_DeltaFrameLimit)
			m_DeltaFrameCount = 0;

		// Actually encode the frame
		CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
		m_Image.Encode(&memFile);	
		//m_Image.Draw(GetDC(NULL));
		lActual = memFile.Tell();	

	} else if (lParam1->lpbiInput->biCompression == FOURCC_YUY2 && lParam1->lpbiInput->biBitCount == 16) {
		// Encode a YUV2 frame
		CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);			
		CompressYUY2DeltaFrame((BYTE *)lParam1->lpInput, &memFile);	
		lActual = memFile.Tell();	
	
	} else if (lParam1->lpbiInput->biCompression == FOURCC_YV12 && lParam1->lpbiInput->biBitCount == 12) {
		// Encode a YUV2 frame
		CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);			
		CompressYV12DeltaFrame((BYTE *)lParam1->lpInput, &memFile);	
		lActual = memFile.Tell();	
	}	

	// Now put the result back	
	lParam1->lpbiOutput->biSizeImage = lActual;
	lParam1->dwFlags = 0;
	*lParam1->lpdwFlags = 0;	

	return ICERR_OK;
};

int VFWhandler::CompressDeltaFrameAuto(ICCOMPRESS* lParam1, DWORD lParam2)
{
	void* VFW_Buffer = NULL;
	DWORD DeltaFrameSize = -1;
	DWORD KeyFrameSize = -1;
	if (CompressDeltaFrame(lParam1, lParam2) == ICERR_OK) {
		DeltaFrameSize = lParam1->lpbiOutput->biSizeImage;		
	} else {
		return CompressKeyFrame(lParam1, lParam2);
	}

	VFW_Buffer = lParam1->lpOutput;
	lParam1->lpOutput = NULL;
	if (CompressKeyFrame(lParam1, lParam2) == ICERR_OK) {
		// Restore the origanl VW buffer
		lParam1->lpOutput = VFW_Buffer;
		KeyFrameSize = lParam1->lpbiOutput->biSizeImage;
		if (KeyFrameSize < DeltaFrameSize) {
			// Keyframe is smaller than a delta frame
			// Replace the delta frame data in the VFW buffer with ours
			memcpy(lParam1->lpOutput, m_MemoryBuffer.GetBuffer(), KeyFrameSize);
			// Reset the delta frame count
			m_DeltaFrameCount = 0;
		} else {
			// Delta frame gets better compression
			// Restore the size (the VFW buffer already has the delta frame data)
			lParam1->lpbiOutput->biSizeImage = DeltaFrameSize;			
			// Set the delta frame flags
			lParam1->dwFlags = 0;
			*lParam1->lpdwFlags = 0;	
		}
	}


	return ICERR_OK;
};
/******************************************************************************
* VFW_compress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
*
* This is used to query if we can compress the specified format. This mostly
* involves checking with Framehandler, since WARP can handle pretty much
* any format once it's preloaded into a WARPframe by Framehandler.
*
******************************************************************************/

int VFWhandler::VFW_compress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;

	//if ((input->biWidth / 2) * 2 != input->biWidth || (input->biHeight / 2) * 2 != input->biHeight)
	//	return ICERR_BADFORMAT;

	if (input->biCompression == FOURCC_YUY2)
		return ICERR_OK;
	if (input->biCompression == FOURCC_YV12)
		return ICERR_OK;

	if (input->biCompression == BI_RGB 
		&& (input->biBitCount == 24 || input->biBitCount == 32)) 
	{
		return ICERR_OK;
	} else {
		return ICERR_BADFORMAT;
	}
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_compress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
*
* Used to get a description of our format for VFW apps. Not that complicated,
* but there's some kludges here. Some extra values WARP needs to know on 
* decompression before decompress are stored in odd places, like the old
* colorspace fourCC from the source video is stored in the output bitmap
* header's BiClrUsed field.
*
******************************************************************************/

int VFWhandler::VFW_compress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;
	
	if(output == NULL)
		return sizeof(BITMAPINFOHEADER)+sizeof(CorePNGCodecPrivate);

	output->biSize = sizeof(BITMAPINFOHEADER)+sizeof(CorePNGCodecPrivate);
	output->biWidth = input->biWidth;
	output->biHeight = input->biHeight;
	output->biCompression = FOURCC_PNG;
	output->biClrUsed = input->biBitCount;
	output->biBitCount = input->biBitCount;

	CorePNGCodecPrivate *codecPrivate = (CorePNGCodecPrivate *)(((BYTE *)output)+sizeof(BITMAPINFOHEADER));
	codecPrivate->wSize = sizeof(CorePNGCodecPrivate);
	if (input->biCompression == FOURCC_YUY2)
		codecPrivate->bType = PNGFrameType_YUY2;
	else if (input->biCompression == FOURCC_YV12)
		codecPrivate->bType = PNGFrameType_YV12;
	else if (input->biCompression == BI_RGB)
		codecPrivate->bType = PNGFrameType_RGB24;

	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_compress_get_size(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
*
* Ok this function is a MAJOR problem. There's no way to determine the maximum
* size of a compressed frame without information from config like timedepth,
* but VFW doesn't guarantee config gets called first. Also, the real answer
* might blow up code that expects a more sane response, not something possibly
* as large as several gigabytes. So, this value gets fudged to what the caller
* would normally expect from another codec. i.e. image resolution * 3.
*
******************************************************************************/

int VFWhandler::VFW_compress_get_size(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	// 150% of uncompressed should be enough for the worst frames
	return m_BufferSize = (input->biHeight * input->biWidth * (input->biBitCount/8))*1.5;
}

/******************************************************************************
* VFW_compress_frames_info(ICCOMPRESSFRAMES* lParam1);
*
* More input info for us if we need it, things like framerate.
*
******************************************************************************/

int VFWhandler::VFW_compress_frames_info(ICCOMPRESSFRAMES* lParam1)
{
	return ICERR_OK;
}

/******************************************************************************
* VFW_compress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
*
* This is basicly notice it's time to begin compression. Thus the config
* should be given to the codec, and any initialization for specific values
* like array sizes are done now.
*
******************************************************************************/

int VFWhandler::VFW_compress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;

	if (input->biCompression == FOURCC_YUY2) {
		// Use the YUV2 encoder
		CreateYUY2(input);

		Y_Channel.SetCompressionFilters(m_PNGFilters);
		Y_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		U_Channel.SetCompressionFilters(m_PNGFilters);
		U_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		V_Channel.SetCompressionFilters(m_PNGFilters);
		V_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
	} else if (input->biCompression == FOURCC_YV12) {
		// Use the YV12 encoder
		CreateYV12(input);

		Y_Channel.SetCompressionFilters(m_PNGFilters);
		Y_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		U_Channel.SetCompressionFilters(m_PNGFilters);
		U_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		V_Channel.SetCompressionFilters(m_PNGFilters);
		V_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
	} else {
		m_Image.Create(input->biWidth, input->biHeight, 24);
		
		m_Image.SetCompressionFilters(m_PNGFilters);
		m_Image.SetCompressionLevel(m_ZlibCompressionLevel);
	}
	m_DeltaFrameCount = 0;
	//mycodec->Configure(*myconfig);	
	return ICERR_OK;

	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_compress_end(int lParam1, int lParam2);
*
* This is the signal to do cleanup once the compress sequence is over.
*
******************************************************************************/

int VFWhandler::VFW_compress_end(int lParam1, int lParam2)
{
	//mycodec.flush();
	return ICERR_OK;
}

/******************************************************************************
* VFW_decompress(ICDECOMPRESS* lParam1, DWORD lParam2);
* 
* The main decompress a frame function.
******************************************************************************/

int VFWhandler::VFW_decompress(ICDECOMPRESS* lParam1, DWORD lParam2)
{	
	VFW_CODEC_CRASH_CATCHER_START;

	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	if (m_CodecPrivate.bType == PNGFrameType_RGB24) {
		// Old-style RGB encoding

		// Preserve the previous frame
		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			m_DeltaFrame = m_Image;
		}

		// Now decompress the frame.
		CxMemFile memFile((BYTE *)lParam1->lpInput, lActual);
		m_Image.Decode(&memFile);		
		//m_Image.Draw(GetDC(NULL));
		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			// Mix the old image and the new image together
			m_DeltaFrame.Mix(m_Image, CxImage::ImageOpType::OpAdd2);
			m_Image = m_DeltaFrame;
		}

		//lParam1->lpbiOutput->biSizeImage = lParam1->lpbiInput->biWidth * lParam1->lpbiInput->biHeight * (lParam1->lpbiInput->biBitCount / 8);

		if (lParam1->lpbiOutput->biBitCount == 32) {
			// Convert the 24-bit+Alpha to 32-bits
			//BITMAPINFOHEADER *decodedImageHeader = (BITMAPINFOHEADER *)m_Image.GetDIB();		
			DWORD sourceHeight = m_Image.GetHeight();
			DWORD sourceWidth = m_Image.GetWidth();
			DWORD effWidth = m_Image.GetWidth()*4;
			BYTE *decodedOutput = (BYTE *)lParam1->lpOutput + (m_Image.GetHeight() - 1) * effWidth;
			RGBQUAD temp;

			/*
			BYTE *imageBits = m_Image.GetBits();
			BYTE *imageAlphaBits = m_Image.GetAlphaBits();
			__asm {
				;Start Height loop
				mov ecx,0 ; Set the loop counter to 0
	_HEIGHT_LOOP:
				; Start Width Loop
				mov esi,0  ; Set the loop counter to 0
	_WIDTH_LOOP:			
				;Copy the pixel colors
				push        ecx ; Push the height into the stack
				push        esi ; Push the width into the stack
				lea         edx,[m_Image]
				push        edx  
				lea         ecx,[ebx+1B8h] 
				call        CxImage::GetPixelColor
				mov         eax,dword ptr [eax] 
				mov         ecx,dword ptr [ebp-14h] 
				mov         dword ptr [edi+sourceWidth*4],eax
				mov         eax,dword ptr [lParam1] 
				
				inc esi ; Increase our width loop counter
				cmp esi, sourceWidth
				jl _WIDTH_LOOP ; Jump back if less than the sourceWidth

				; End Width Loop
				; Get ready for next line
				lea         edx,[sourceWidth*4]
				sub         edi,edx 
				mov         edx,dword ptr [ebp-18h] 			

				inc ecx ; Increase our height loop counter
				cmp ecx, sourceHeight
				jl _HEIGHT_LOOP ; Jump back if less than the sourceHeight
				; End Height Loop
			};
			*/

			for (DWORD height = 0; height < sourceHeight; height++) {
				for (DWORD width = 0; width < sourceWidth; width++) {
					//temp = m_Image.GetPixelColor(width, height);
					((RGBQUAD *)decodedOutput)[width] = m_Image.GetPixelColor(width, height);			
				}
				decodedOutput -= effWidth;
			}
		} else if (lParam1->lpbiOutput->biBitCount == 24) {
			memcpy(lParam1->lpOutput, m_Image.GetBits(), lParam1->lpbiOutput->biSizeImage);
		}
	} else if (m_CodecPrivate.bType == PNGFrameType_YUY2) {
		assert(lParam1->lpbiOutput->biCompression == FOURCC_YUY2);

		// Preserve the previous frame
		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			Y_Channel_Delta = Y_Channel;
			U_Channel_Delta = U_Channel;
			V_Channel_Delta = V_Channel;
		}

		// Now decompress the frame.
		CxMemFile memFile((BYTE *)lParam1->lpInput, lActual);
		Y_Channel.Decode(&memFile);		
		U_Channel.Decode(&memFile);		
		V_Channel.Decode(&memFile);		
		
		//Y_Channel.Draw(GetDC(NULL));
		//U_Channel.Draw(GetDC(NULL));
		//V_Channel.Draw(GetDC(NULL));
		//Y_Channel_Delta.Draw(GetDC(NULL));
		//U_Channel_Delta.Draw(GetDC(NULL));
		//V_Channel_Delta.Draw(GetDC(NULL));

		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			// Mix the old image and the new image together
			BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
			BYTE *U_data = (BYTE *)U_Channel.GetBits();
			BYTE *V_data = (BYTE *)V_Channel.GetBits();

			BYTE *Y_Delta_data = (BYTE *)Y_Channel_Delta.GetBits();
			BYTE *U_Delta_data = (BYTE *)U_Channel_Delta.GetBits();
			BYTE *V_Delta_data = (BYTE *)V_Channel_Delta.GetBits();

			// Copy and Compare to the previous frame
			for (DWORD height = 0; height < Y_Channel.GetHeight()*2; height++) {
				for (DWORD width = 0; width < Y_Channel.GetWidth(); width += 4) {
					Y_data[0] = Y_data[0] + Y_Delta_data[0];
					Y_data++;
					Y_Delta_data++;
					
					U_data[0] = U_data[0] + U_Delta_data[0];
					U_data++;
					U_Delta_data++;
					
					Y_data[0] = Y_data[0] + Y_Delta_data[0];
					Y_data++;
					Y_Delta_data++;
					
					V_data[0] = V_data[0] + V_Delta_data[0];					
					V_data++;
					V_Delta_data++;
				}				
			}		

			Y_Channel_Delta = Y_Channel;
			U_Channel_Delta = U_Channel;
			V_Channel_Delta = V_Channel;
		}

		// Merge and Copy the decoded YUV channels into the output buffer
		BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
		BYTE *U_data = (BYTE *)U_Channel.GetBits();
		BYTE *V_data = (BYTE *)V_Channel.GetBits();
		BYTE *outputYUV2Data = (BYTE *)lParam1->lpOutput;
		
		//memset(lParam1->lpOutput, 0, lParam1->lpbiOutput->biSizeImage);
		
		for (DWORD height = 0; height < Y_Channel.GetHeight()*2; height++) {
			for (DWORD width = 0; width < Y_Channel.GetWidth(); width += 4) {
				outputYUV2Data[width+0] = Y_data++[0];
				outputYUV2Data[width+1] = U_data++[0];
				outputYUV2Data[width+2] = Y_data++[0];
				outputYUV2Data[width+3] = V_data++[0];
			}
			outputYUV2Data += width;
		}
	} else if (m_CodecPrivate.bType == PNGFrameType_YV12) {
		assert(lParam1->lpbiOutput->biCompression == FOURCC_YV12);

		// Preserve the previous frame
		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			Y_Channel_Delta = Y_Channel;
			U_Channel_Delta = U_Channel;
			V_Channel_Delta = V_Channel;
		}

		// Now decompress the frame.
		CxMemFile memFile((BYTE *)lParam1->lpInput, lActual);
		Y_Channel.Decode(&memFile);		
		U_Channel.Decode(&memFile);		
		V_Channel.Decode(&memFile);		
		
		//Y_Channel.Draw(GetDC(NULL));
		//U_Channel.Draw(GetDC(NULL));
		//V_Channel.Draw(GetDC(NULL));

		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			// Mix the old image and the new image together
			BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
			BYTE *U_data = (BYTE *)U_Channel.GetBits();
			BYTE *V_data = (BYTE *)V_Channel.GetBits();

			BYTE *Y_Delta_data = (BYTE *)Y_Channel_Delta.GetBits();
			BYTE *U_Delta_data = (BYTE *)U_Channel_Delta.GetBits();
			BYTE *V_Delta_data = (BYTE *)V_Channel_Delta.GetBits();

			for (DWORD d = 0; d < Y_Channel.GetSizeImage(); d++)
				Y_data[0] = Y_data++[0] + Y_Delta_data++[0];
			
			for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
				U_data[0] = U_data++[0] + U_Delta_data++[0];
			
			for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
				V_data[0] = U_data++[0] + V_Delta_data++[0];

			// Copy and Compare to the previous frame
			/*for (DWORD height = 0; height < Y_Channel.GetHeight()*2; height++) {
				for (DWORD width = 0; width < Y_Channel.GetWidth(); width += 4) {
					Y_data++[0] = Y_data[width+0] + Y_Delta_data++[0];
					U_data++[0] = U_data[width+1] + U_Delta_data++[0];
					Y_data++[0] = Y_data[width+2] + Y_Delta_data++[0];
					V_data++[0] = V_data[width+3] + V_Delta_data++[0];					
				}				
			}		*/

			Y_Channel_Delta = Y_Channel;
			U_Channel_Delta = U_Channel;
			V_Channel_Delta = V_Channel;
		}

		// Merge and Copy the decoded YUV channels into the output buffer
		BYTE *outputYUV2Data = (BYTE *)lParam1->lpOutput;
		memcpy(outputYUV2Data, Y_Channel.GetBits(), Y_Channel.GetSizeImage());
		outputYUV2Data += Y_Channel.GetSizeImage();
		memcpy(outputYUV2Data, U_Channel.GetBits(), U_Channel.GetSizeImage());
		outputYUV2Data += U_Channel.GetSizeImage();
		memcpy(outputYUV2Data, V_Channel.GetBits(), V_Channel.GetSizeImage());
		
		
		//memset(lParam1->lpOutput, 0, lParam1->lpbiOutput->biSizeImage);				
	}
	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_decompress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
* 
* Checks if we can decompress the incoming format. Since we can only 
* decompress our own format, we just check for that fourcc.
*
******************************************************************************/

int VFWhandler::VFW_decompress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;
	if (input->biCompression == FOURCC_PNG || input->biCompression == FOURCC_PNG_OLD)
	{
		if (output != NULL) {
			BYTE type = PNGFrameType_RGB24;
			if (input->biSize > sizeof(BITMAPINFOHEADER)) {
				type = ((CorePNGCodecPrivate *)(((BYTE *)input)+sizeof(BITMAPINFOHEADER)))->bType;
			}
			if (type == PNGFrameType_RGB24) {
				if (output->biCompression == BI_RGB) {
					if (!m_DecodeToRGB24) {
						if (output->biBitCount != 24 && output->biBitCount != 32)
							// Bad bit-depth
							return ICERR_BADFORMAT;
					} else {
						// Only decode 24-bits
						if (output->biBitCount != 24)
							// Bad bit-depth
							return ICERR_BADFORMAT;
					}
				} else {
					// Only RGB out is support
					return ICERR_BADFORMAT;
				}
			} else if (type == PNGFrameType_YUY2) {
				if (output->biCompression != FOURCC_YUY2)
					return ICERR_BADFORMAT;
			} else if (type == PNGFrameType_YV12) {
				if (output->biCompression != FOURCC_YV12)
					return ICERR_BADFORMAT;
			} else {
				// Not RGB or YUV
				return ICERR_BADFORMAT;
			}
		}
		return ICERR_OK;
	
	}	else {
		return ICERR_BADFORMAT;
	}
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_decompress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
*
* This is used to indicate what our output format after decompress will be
* like. Note the original fourcc of the input was stored in biClrUsed, so
* here we restore it to the output's biCompression field.
*
******************************************************************************/

int VFWhandler::VFW_decompress_get_format(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;
	if (output != NULL) {		
		output->biSize = input->biSize;
		output->biWidth = input->biWidth;
		output->biHeight = input->biHeight;
		output->biBitCount = input->biBitCount;
		output->biSizeImage = input->biSizeImage;
		// RGB compressed or old-style (RGB Only)?
		if (input->biSize >= sizeof(BITMAPINFOHEADER) 
			|| ((CorePNGCodecPrivate *)((BYTE *)input)+sizeof(BITMAPINFOHEADER))->bType == PNGFrameType_RGB24)
		{
			output->biCompression = BI_RGB;
			if (m_DecodeToRGB24) {
				output->biBitCount = 24;				
			}
		} else {
			// Ok we check which other type is it
			CorePNGCodecPrivate *codecPrivate = (CorePNGCodecPrivate *)((BYTE *)input)+sizeof(BITMAPINFOHEADER);
			if (codecPrivate->bType == PNGFrameType_YUY2) {
				output->biCompression = FOURCC_YUY2;
				output->biBitCount = 16;
				output->biSizeImage = output->biWidth * output->biHeight * output->biBitCount;
			} else if (codecPrivate->bType == PNGFrameType_YV12) {
				output->biCompression = FOURCC_YV12;
				output->biBitCount = 12;
				output->biSizeImage = output->biWidth * output->biHeight * output->biBitCount;
			} else {
				return ICERR_BADFORMAT;
			}
		}
	}
	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_decompress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
*
* This is basicly notice it's time to begin decompression. Thus the config
* should be given to the codec, and any initialization for specific values
* like array sizes are done now.
*
******************************************************************************/

int VFWhandler::VFW_decompress_begin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
	VFW_CODEC_CRASH_CATCHER_START;

	if (input->biSize == sizeof(BITMAPINFOHEADER) + sizeof(CorePNGCodecPrivate)) {
		memcpy(&m_CodecPrivate, ((BYTE *)input)+sizeof(BITMAPINFOHEADER), sizeof(CorePNGCodecPrivate));
	} else {
		m_CodecPrivate = CorePNGCodecPrivate();
	}

	// Double-check the output format
	if (m_CodecPrivate.bType == PNGFrameType_RGB24) {
		if (output->biCompression != BI_RGB)
			return ICERR_BADFORMAT;

	} else if (m_CodecPrivate.bType == PNGFrameType_YUY2) {
		if (output->biCompression != FOURCC_YUY2)
			return ICERR_BADFORMAT;

	} else if (m_CodecPrivate.bType == PNGFrameType_YV12) {
		if (output->biCompression != FOURCC_YV12)
			return ICERR_BADFORMAT;
	}

	m_Image.Create(input->biWidth, input->biHeight, 24);
	//mycodec->Configure(*myconfig);
	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
}

/******************************************************************************
* VFW_decompress_end(int lParam1, int lParam2);
*
* This is the signal to do cleanup once the decompress sequence is over.
*
******************************************************************************/

int VFWhandler::VFW_decompress_end(int lParam1, int lParam2)
{
	//mycodec.flush();
	return ICERR_OK;
}

inline RGBQUAD VFWhandler::AveragePixels(DWORD x, DWORD y) {
	RGBQUAD previousPixel;
	WORD avgRed = 255;						
	WORD avgGreen = 255;						
	WORD avgBlue = 255;
	WORD avgCount = 1;						
	if (x > 0) {
		previousPixel = m_Image.GetPixelColor(x-1, y);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
		if (y > 0) {
			previousPixel = m_Image.GetPixelColor(x-1, y-1);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
		if (y < m_Image.GetHeight()) {
			previousPixel = m_Image.GetPixelColor(x-1, y+1);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
	}
	if (x < m_Image.GetWidth()) {
		previousPixel = m_Image.GetPixelColor(x+1, y);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
		if (y > 0) {
			previousPixel = m_Image.GetPixelColor(x+1, y-1);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
		if (y < m_Image.GetHeight()) {
			previousPixel = m_Image.GetPixelColor(x+1, y+1);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
	}
	if (y > 0) {
		previousPixel = m_Image.GetPixelColor(x, y-1);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
	}						
	if (y < m_Image.GetHeight()) {
		previousPixel = m_Image.GetPixelColor(x, y+1);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
	}

	if (x > 1) {
		previousPixel = m_Image.GetPixelColor(x-2, y);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
		if (y > 1) {
			previousPixel = m_Image.GetPixelColor(x-2, y-2);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
		if (y < m_Image.GetHeight()-1) {
			previousPixel = m_Image.GetPixelColor(x-2, y+2);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
	}
	if (x < m_Image.GetWidth()-1) {
		previousPixel = m_Image.GetPixelColor(x+2, y);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
		if (y > 1) {
			previousPixel = m_Image.GetPixelColor(x+2, y-2);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
		if (y < m_Image.GetHeight()-1) {
			previousPixel = m_Image.GetPixelColor(x+2, y+2);
			avgRed += previousPixel.rgbRed;
			avgBlue += previousPixel.rgbBlue;
			avgGreen += previousPixel.rgbGreen;
			avgCount++;
		}
	}
	if (y > 1) {
		previousPixel = m_Image.GetPixelColor(x, y-2);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
	}						
	if (y < m_Image.GetHeight()-1) {
		previousPixel = m_Image.GetPixelColor(x, y+2);
		avgRed += previousPixel.rgbRed;
		avgBlue += previousPixel.rgbBlue;
		avgGreen += previousPixel.rgbGreen;
		avgCount++;
	}
	previousPixel.rgbRed = avgRed / avgCount;
	previousPixel.rgbGreen = avgGreen / avgCount;
	previousPixel.rgbBlue = avgBlue / avgCount;
	previousPixel.rgbReserved = 0;	

	return previousPixel;
};

BOOL VFWhandler::ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{	
  switch (uMsg)
  {
		case WM_INITDIALOG:
		{
			INITCOMMONCONTROLSEX common;			
			common.dwICC = ICC_UPDOWN_CLASS; 
			common.dwSize = sizeof(common);
			InitCommonControlsEx(&common);

			HRSRC hres = FindResource(g_hInst, MAKEINTRESOURCE(IDR_PNG_LOGO), "PNG");
			HGLOBAL hgImage = LoadResource(g_hInst, hres);
			myLogo.Decode((BYTE *)LockResource(hgImage), SizeofResource(g_hInst, hres), CXIMAGE_FORMAT_PNG);
#ifdef _DEBUG
			RGBQUAD blackColor = {0, 0, 0, 0};
			myLogo.DrawTextOnImage(GetDC(hwndDlg), 240, 20, "Debug Build", blackColor, "Arial", 18, 10000);
#endif

			CheckDlgButton(hwndDlg, IDC_CHECK_DECODE_RGB24, m_DecodeToRGB24);
			CheckDlgButton(hwndDlg, IDC_CHECK_DELTA_FRAMES, m_DeltaFramesEnabled);
			CheckDlgButton(hwndDlg, IDC_CHECK_AUTO_DELTA_FRAMES, m_DeltaFrameAuto);
						
			SendDlgItemMessage(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL, UDM_SETRANGE, 0, (LPARAM)MAKELONG(1800, 1));
			SendDlgItemMessage(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL, UDM_SETPOS, 0, m_DeltaFrameLimit+1);

			SendDlgItemMessage(hwndDlg,	IDC_SPIN_DROP_THRESHOLD, UDM_SETRANGE, 0, (LPARAM)MAKELONG(10, 0));
			SendDlgItemMessage(hwndDlg,	IDC_SPIN_DROP_THRESHOLD, UDM_SETPOS, 0, m_DropFrameThreshold);

			SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_ADDSTRING, 0, (LPARAM)"1 - Fastest");
			SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_ADDSTRING, 0, (LPARAM)"6 - Normal");
			SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_ADDSTRING, 0, (LPARAM)"9 - Best");

			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_NO_FILTERS, !m_PNGFilters);						
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_NONE, (m_PNGFilters & PNG_FILTER_NONE));
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_AVG, (m_PNGFilters & PNG_FILTER_AVG));
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_PAETH, (m_PNGFilters & PNG_FILTER_PAETH));
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_SUB, (m_PNGFilters & PNG_FILTER_SUB));
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_UP, (m_PNGFilters & PNG_FILTER_UP));
			CheckDlgButton(hwndDlg, IDC_CHECK_PNG_ALL_FILTERS, (m_PNGFilters == PNG_ALL_FILTERS));			
			switch (m_ZlibCompressionLevel)
			{
				case 1:
					SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_SETCURSEL, 0, 0);
					break;
				case 6:
					SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_SETCURSEL, 1, 0);
					break;
				case 9:
					SendDlgItemMessage(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL, CB_SETCURSEL, 2, 0);
					break;				
			};
			break;
		}		
		case WM_PAINT:
		{			
			myLogo.Draw(GetDC(hwndDlg));		
			break;
		}
		case WM_DESTROY:
		{			
			myLogo.Destroy();
			break;
		}
		case WM_COMMAND:
			// Process button
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_OK:
					m_PNGFilters = 0;
					if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_NO_FILTERS) == BST_CHECKED) {
						m_PNGFilters = PNG_NO_FILTERS;
					} else {
						// We look at our other filters
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_ALL_FILTERS) == BST_CHECKED)
							m_PNGFilters |= PNG_ALL_FILTERS;
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_FILTER_NONE) == BST_CHECKED)
							m_PNGFilters |= PNG_FILTER_NONE;
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_FILTER_AVG) == BST_CHECKED)
							m_PNGFilters |= PNG_FILTER_AVG;
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_FILTER_PAETH) == BST_CHECKED)
							m_PNGFilters |= PNG_FILTER_PAETH;
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_FILTER_SUB) == BST_CHECKED)
							m_PNGFilters |= PNG_FILTER_SUB;
						if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_FILTER_UP) == BST_CHECKED)
							m_PNGFilters |= PNG_FILTER_UP;
					}
					m_ZlibCompressionLevel = GetDlgItemInt(hwndDlg, IDC_COMBO_COMPRESSION_LEVEL, NULL, FALSE);
					m_DecodeToRGB24 = IsDlgButtonChecked(hwndDlg, IDC_CHECK_DECODE_RGB24);
					m_DeltaFramesEnabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_DELTA_FRAMES);
					m_DeltaFrameAuto = IsDlgButtonChecked(hwndDlg, IDC_CHECK_AUTO_DELTA_FRAMES);
					m_DropFrameThreshold = SendDlgItemMessage(hwndDlg,	IDC_SPIN_DROP_THRESHOLD, UDM_GETPOS, 0, 0);
					m_DeltaFrameLimit = SendDlgItemMessage(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL, UDM_GETPOS, 0, 0)-1;
					
					SaveSettings();

					EndDialog(hwndDlg, IDOK);
					break;
				case IDC_CHECK_PNG_ALL_FILTERS:
					if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_PNG_ALL_FILTERS) == BST_CHECKED) {
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_NO_FILTERS, BST_UNCHECKED);
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_AVG, BST_CHECKED);
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_PAETH, BST_CHECKED);
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_SUB, BST_CHECKED);
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_UP, BST_CHECKED);
						CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_NONE, BST_CHECKED);

					}
					break;
				case IDC_CHECK_PNG_NO_FILTERS:
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_ALL_FILTERS, BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_AVG, BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_PAETH, BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_SUB, BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_UP, BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_CHECK_PNG_FILTER_NONE, BST_UNCHECKED);					
					break;
				case IDC_BUTTON_CANCEL:
					EndDialog(hwndDlg, IDCANCEL);
					break;
			}
			break;
		default:
			break;
			//Nothing for now
	}
  return FALSE;
}

BOOL CALLBACK ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	VFWhandler *vfwData = (VFWhandler *)GetWindowLong(hwndDlg, DWL_USER);	
  if (uMsg == WM_INITDIALOG) {
		// Store the VFWhandler
		vfwData = (VFWhandler *)lParam;
		SetWindowLong(hwndDlg, DWL_USER, (LONG)vfwData);
	}
	if (vfwData != NULL) {
		return vfwData->ConfigurationDlgProc(hwndDlg, uMsg, wParam, lParam);
	}
	return FALSE;
};

DWORD CorePNG_GetRegistryValue(char *value_key, DWORD default_value)
{	
	char *reg_key = "Software\\CorePNG\\";
	DWORD ret_value = default_value;
	HKEY key_handle = NULL;
	DWORD lpType = NULL;
	DWORD state = 0;
	DWORD size = sizeof(ret_value);

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_handle, &state);
	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, temp, 0, reg_key, 1024, NULL); 
	if(state == REG_OPENED_EXISTING_KEY)
		RegQueryValueEx(key_handle, value_key, 0, &lpType, (BYTE*)&ret_value, &size);	
	else if (state == REG_CREATED_NEW_KEY)
		//We write the default value
		RegSetValueEx(key_handle, value_key, 0, REG_DWORD, (CONST BYTE*)&ret_value, size);

	RegCloseKey(key_handle);
	return ret_value;
}

void CorePNG_SetRegistryValue(char *value_key, DWORD the_value)
{
	char *reg_key = "SOFTWARE\\CorePNG\\";
	HKEY key_handle = NULL;
	DWORD state = 0;
	SECURITY_ATTRIBUTES sa = {sizeof(sa), 0,1};

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_key, 0, "", 0, KEY_WRITE, &sa, &key_handle, &state);

	DWORD size = sizeof(the_value);
	RegSetValueEx(key_handle, value_key, 0, REG_DWORD, (CONST BYTE*)&the_value, size);
	//char *err_key = new char[1024];
	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, temp, 0, err_key, 1024, NULL); 
	RegCloseKey(key_handle);
};
