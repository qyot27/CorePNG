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
/*
	About 768 byte of 256 palette overhead per image.

*/

#include "VFWhandler.h"

extern HINSTANCE g_hInst;

typedef BOOL (*SetLayeredWindowAttributes_def)(HWND, COLORREF, BYTE, DWORD);

VFWhandler::VFWhandler(void)
{
	ODS("VFWhandler::VFWhandler()");
	
	// Load defaults from reg
	LoadSettings();

	m_TotalKeyframes = 0;
	m_TotalDeltaframes = 0;
	m_hwndStatusDialog = NULL;
	m_BufferSize = 0;
	m_DeltaFrameCount = 0;
	m_Output_Mode = PNGOutputMode_None;
	m_threadInfo = NULL;	
	m_MemoryBuffer.Open();
}

VFWhandler::~VFWhandler(void)
{
	ODS("VFWhandler::~VFWhandler()");

}

void VFWhandler::LoadSettings() {	
	m_ZlibCompressionLevel = CorePNG_GetRegistryValue("zlib Compression Level", 6);
	m_PNGFilters = CorePNG_GetRegistryValue("PNG Filters", PNG_ALL_FILTERS);
	m_DeltaFramesEnabled = CorePNG_GetRegistryValue("Use Delta Frames", 0);
	m_DeltaFrameAuto = CorePNG_GetRegistryValue("Auto Delta Frames", 0);
	m_DeltaFrameLimit = CorePNG_GetRegistryValue("Keyframe Interval", 30);
	m_DropFrameThreshold = CorePNG_GetRegistryValue("Drop Frame Threshold", 0);
	m_DecodeToRGB24 = CorePNG_GetRegistryValue("Always Decode to RGB24", 0);
	m_EnableMultiThreading = CorePNG_GetRegistryValue("Multi-threading", 0);
	m_EnableStatusDialog = CorePNG_GetRegistryValue("Status Dialog", 0);
}

void VFWhandler::SaveSettings() {	
	CorePNG_SetRegistryValue("zlib Compression Level", m_ZlibCompressionLevel);
	CorePNG_SetRegistryValue("PNG Filters", m_PNGFilters);
	CorePNG_SetRegistryValue("Use Delta Frames", m_DeltaFramesEnabled);
	CorePNG_SetRegistryValue("Auto Delta Frames", m_DeltaFrameAuto);
	CorePNG_SetRegistryValue("Keyframe Interval", m_DeltaFrameLimit);	
	CorePNG_SetRegistryValue("Drop Frame Threshold", m_DropFrameThreshold);
	CorePNG_SetRegistryValue("Always Decode to RGB24", m_DecodeToRGB24);
	CorePNG_SetRegistryValue("Multi-threading", m_EnableMultiThreading);
	CorePNG_SetRegistryValue("Status Dialog", m_EnableStatusDialog);
}

void VFWhandler::VFW_about(HWND hParentWindow)
{
	ODS("VFWhandler::VFW_about()");
	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hParentWindow, ::AboutDlgProc, (LPARAM)this);		
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
	ODS("VFWhandler::VFW_configure()");
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
	ODS("VFWhandler::VFW_compress()");
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
	
	// Increase the keyframe count
	m_TotalKeyframes++;

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
			CopyImageBits(m_DeltaFrame, m_Image);
			m_DeltaFrameCount++;
		}
#ifdef QUADRANT_SCAN_TEST
		int dummy = 0;
		int **scantable = NULL;
		scantable = new int*[1024+1];
		memset(scantable, 0, sizeof(int)*1024);
		for (DWORD h = 0; h < 1024; h++) {
			scantable[h] = new int[1024+1];
			memset(scantable[h], 0, sizeof(int)*1024);
		}
		for (DWORD h = 0; h < m_Image.GetHeight(); h++) {
			for (DWORD w = 0; w < m_Image.GetWidth(); w++) {
				scantable[w][h] = 1;
			}
		}
		QuadrantScan(scantable, 0, 0, 1024, 1024, 1024, dummy);
		{
			CxIOFile textFile;
			char txt_buffer[1024];
			textFile.Open("D:\\test.txt", "w");
			for (DWORD h = 0; h < 1024; h++) {
				for (DWORD w = 0; w < 1024; w++) {
					wsprintf(txt_buffer, "%i ", scantable[w][h]);
					textFile.Write(txt_buffer, lstrlen(txt_buffer), 1);
				}
				textFile.Write("\n", 1, 1);
			}
		}
		CxImagePNG scanImage = m_Image;
		for (DWORD y1 = 0; y1 < m_Image.GetHeight(); y1++) {
			for (DWORD x1 = 0; x1 < m_Image.GetWidth(); x1++) {
				int x2, y2 = 0;
				if (scantable[x1][y1] > m_Image.GetWidth()) {
					y2 = scantable[x1][y1] / m_Image.GetWidth();
				} else {
					y2 = 0;
				}
				x2 =  scantable[x1][y1] - y2 * m_Image.GetWidth();
				m_Image.SetPixelColor(x2, y2, scanImage.GetPixelColor(x1, y1));
			}
		}
		// Recreate the image
		for (DWORD y1 = 0; y1 < m_Image.GetHeight(); y1++) {
			for (DWORD x1 = 0; x1 < m_Image.GetWidth(); x1++) {
				int x2, y2 = 0;
				if (scantable[x1][y1] > m_Image.GetWidth()) {
					y2 = scantable[x1][y1] / m_Image.GetWidth();
				} else {
					y2 = 0;
				}
				x2 =  scantable[x1][y1] - y2 * m_Image.GetWidth();
				scanImage.SetPixelColor(x1, y1, m_Image.GetPixelColor(x2, y2));				
				
			}
		}
		m_Image.Draw(GetDC(NULL));
		scanImage.Draw(GetDC(NULL));
		
		m_Image.SetCompressionFilters(m_PNGFilters);
		m_Image.SetCompressionLevel(m_ZlibCompressionLevel);
		scanImage.SetCompressionFilters(m_PNGFilters);
		scanImage.SetCompressionLevel(m_ZlibCompressionLevel);
		scanImage.RotateLeft();
		m_Image.RotateLeft();
		m_Image.Save("D:\\kim_quadrant.png", CXIMAGE_FORMAT_PNG);
		scanImage.Save("D:\\kim_orig.png", CXIMAGE_FORMAT_PNG);		
#endif

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
	} else if ((lParam1->lpbiInput->biCompression == FOURCC_YUY2 || lParam1->lpbiInput->biCompression == FOURCC_YUYV)&& lParam1->lpbiInput->biBitCount == 16) {
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
	
	// Increase the delta-frame count
	m_TotalDeltaframes++;

	if (lParam1->lpbiInput->biBitCount == 24) {
		if (m_DropFrameThreshold != 0) {
			DWORD imageDiff = abs(memcmp(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual));
			if (imageDiff < m_DropFrameThreshold) {
				// Drop this frame
				if (lParam2 == 1) {
					// Multi-thread call
					lParam1->dwFrameSize = 0;
				} else {
					lParam1->lpbiOutput->biSizeImage = 0;
				}
				lParam1->dwFlags = 0;
				*lParam1->lpdwFlags = 0;					
				// Increase the delta-frame count even though we drop this frame,
				// or else we could have long time periods without keyframes
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
				if (lParam2 == 1) {
					// Multi-thread call
					lParam1->dwFrameSize = 0;
				} else {
					lParam1->lpbiOutput->biSizeImage = 0;
				}
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

	} else if ((lParam1->lpbiInput->biCompression == FOURCC_YUY2 || lParam1->lpbiInput->biCompression == FOURCC_YUYV)&& lParam1->lpbiInput->biBitCount == 16) {
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
	if (lParam2 == 1) {
		// Multi-thread call
		lParam1->dwFrameSize = lActual;
	} else {
		lParam1->lpbiOutput->biSizeImage = lActual;
	}
	lParam1->dwFlags = 0;
	*lParam1->lpdwFlags = 0;	

	return ICERR_OK;
};

int VFWhandler::CompressDeltaFrameAuto(ICCOMPRESS* lParam1, DWORD lParam2)
{
	void* VFW_Buffer = NULL;
	DWORD DeltaFrameSize = -1;
	DWORD KeyFrameSize = -1;
	if (m_EnableMultiThreading) {				
		memcpy(&m_threadInfo->frameData, lParam1, sizeof(ICCOMPRESS));
		if (m_threadInfo->frameData.lFrameNum == -1)
			m_threadInfo->frameData.lFrameNum == 0;
		LeaveCriticalSection(&m_threadInfo->csFrameData);
	} else {
		if (CompressDeltaFrame(lParam1, lParam2) == ICERR_OK) {
			DeltaFrameSize = lParam1->lpbiOutput->biSizeImage;		
		} else {
			return CompressKeyFrame(lParam1, lParam2);
		}
	}

	VFW_Buffer = lParam1->lpOutput;
	lParam1->lpOutput = NULL;
	if (CompressKeyFrame(lParam1, lParam2) == ICERR_OK) {
		// Restore the origanl VW buffer
		lParam1->lpOutput = VFW_Buffer;
		KeyFrameSize = lParam1->lpbiOutput->biSizeImage;
		
		if (m_EnableMultiThreading) {
			// Ok, wait till the other thread is done compressing
			EnterCriticalSection(&m_threadInfo->csFrameData);
			DeltaFrameSize = m_threadInfo->DeltaFrameSize;
			m_threadInfo->DeltaFrameSize = -1;			
		}

		if (KeyFrameSize < DeltaFrameSize) {
			// Keyframe is smaller than a delta frame
			m_TotalDeltaframes--;
			// Replace the delta frame data in the VFW buffer with ours
			memcpy(lParam1->lpOutput, m_MemoryBuffer.GetBuffer(), KeyFrameSize);
			// Reset the delta frame count
			m_DeltaFrameCount = 0;
		} else {
			// Delta frame gets better compression
			m_TotalKeyframes--;
			// Restore the size (the VFW buffer already has the delta frame data)
			lParam1->lpbiOutput->biSizeImage = DeltaFrameSize;			
			// Set the delta frame flags
			lParam1->dwFlags = 0;
			*lParam1->lpdwFlags = 0;	
		}
	}


	return ICERR_OK;
};

bool VFWhandler::CreateYUY2(BITMAPINFOHEADER* input)
{
	m_Height = input->biHeight;	
	m_Width = input->biWidth;
	
	Y_Channel.Create(m_Width, m_Height, 8);
	U_Channel.Create(m_Width/2, m_Height, 8);
	V_Channel.Create(m_Width/2, m_Height, 8);
	
	Y_Channel.GrayScale();
	U_Channel.GrayScale();
	V_Channel.GrayScale();

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

	for (DWORD height = 0; height < m_Height*2; height++) {
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
	for (DWORD height = 0; height < m_Height*2; height++) {
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
	m_Height = input->biHeight;
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
	memcpy(Y_Channel.GetBits(), inputYUV2Data, Y_Channel.GetSizeImage());
	inputYUV2Data += Y_Channel.GetSizeImage();
	memcpy(V_Channel.GetBits(), inputYUV2Data, V_Channel.GetSizeImage());
	inputYUV2Data += V_Channel.GetSizeImage();
	memcpy(U_Channel.GetBits(), inputYUV2Data, U_Channel.GetSizeImage());

	if (m_DeltaFramesEnabled) {
		CopyImageBits(Y_Channel_Delta, Y_Channel);
		CopyImageBits(U_Channel_Delta, U_Channel);
		CopyImageBits(V_Channel_Delta, V_Channel);
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

void VFWhandler::CopyImageBits(CxImage &dest, CxImage &src)
{
	// Do a debug check if the image formats match
	assert(dest.GetSizeImage() == src.GetSizeImage());
	// Do a direct memory copy
	memcpy(dest.GetBits(), src.GetBits(), src.GetSizeImage());
}

void __cdecl VFWhandler::DeltaFrameCompressThread(void *threadData)
{
	DeltaThreadInfo *threadInfo = (DeltaThreadInfo *)threadData;
	
	while (threadInfo->bRunning) {
		while (threadInfo->DeltaFrameSize != -1) Sleep(10);		
		EnterCriticalSection(&threadInfo->csFrameData);
		if (threadInfo->frameData.lFrameNum != -1) {
			threadInfo->handler->CompressDeltaFrame(&threadInfo->frameData, 1);
			threadInfo->DeltaFrameSize = threadInfo->frameData.dwFrameSize;
			threadInfo->frameData.lFrameNum = -1;
		}
		LeaveCriticalSection(&threadInfo->csFrameData);
	}

	DeleteCriticalSection(&threadInfo->csFrameData);
	delete threadInfo;
	_endthread();
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
	ODS("VFWhandler::VFW_compress_query()");
	VFW_CODEC_CRASH_CATCHER_START;

	//if ((input->biWidth / 2) * 2 != input->biWidth || (input->biHeight / 2) * 2 != input->biHeight)
	//	return ICERR_BADFORMAT;

	if (input->biCompression == FOURCC_YUY2 || input->biCompression == FOURCC_YUYV)
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
	ODS("VFWhandler::VFW_compress_get_format()");
	VFW_CODEC_CRASH_CATCHER_START;
	
	if(output == NULL)
		return sizeof(BITMAPINFOHEADER)+sizeof(CorePNGCodecPrivate);

	output->biSize = sizeof(BITMAPINFOHEADER)+sizeof(CorePNGCodecPrivate);
	output->biWidth = input->biWidth;
	output->biHeight = input->biHeight;
	output->biCompression = FOURCC_PNG;
	output->biPlanes = input->biPlanes;
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
	ODS("VFWhandler::VFW_compress_get_size()");
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
	ODS("VFWhandler::VFW_compress_frames_info()");
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
	ODS("VFWhandler::VFW_compress_begin()");
	VFW_CODEC_CRASH_CATCHER_START;

	if (input->biCompression == FOURCC_YUY2) {
		m_Input_Mode = FOURCC_YUY2;
		// Use the YUV2 encoder
		CreateYUY2(input);

		Y_Channel.SetCompressionFilters(m_PNGFilters);
		Y_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		U_Channel.SetCompressionFilters(m_PNGFilters);
		U_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		V_Channel.SetCompressionFilters(m_PNGFilters);
		V_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
	} else if (input->biCompression == FOURCC_YV12) {
		m_Input_Mode = FOURCC_YV12;
		// Use the YV12 encoder
		CreateYV12(input);

		Y_Channel.SetCompressionFilters(m_PNGFilters);
		Y_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		U_Channel.SetCompressionFilters(m_PNGFilters);
		U_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
		V_Channel.SetCompressionFilters(m_PNGFilters);
		V_Channel.SetCompressionLevel(m_ZlibCompressionLevel);
	} else {
		m_Input_Mode = BI_RGB;
		m_Image.Create(input->biWidth, input->biHeight, 24);
		m_DeltaFrame = m_Image;

		m_Image.SetCompressionFilters(m_PNGFilters);
		m_Image.SetCompressionLevel(m_ZlibCompressionLevel);
	}
	m_DeltaFrameCount = 0;
	m_TotalKeyframes = 0;
	m_TotalDeltaframes = 0;

	if (m_EnableMultiThreading) {
		m_threadInfo = new DeltaThreadInfo;
		InitializeCriticalSection(&m_threadInfo->csFrameData);
		m_threadInfo->bRunning = true;
		m_threadInfo->handler = this;
		m_threadInfo->DeltaFrameSize = -1;
		m_threadInfo->frameData.lFrameNum = -1;
		_beginthread(DeltaFrameCompressThread, 0, (void *)m_threadInfo);
		Sleep(33);
		// Block the thread until we really need it
		EnterCriticalSection(&m_threadInfo->csFrameData);
	}

	if (m_EnableStatusDialog) {
		m_hwndStatusDialog = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_STATUS), NULL, ::StatusDlgProc, (LPARAM)this);
	}
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
	ODS("VFWhandler::VFW_compress_end()");
	if (m_EnableMultiThreading) {				
		// We leave the thread to exit itself
		m_threadInfo->bRunning = false;
		LeaveCriticalSection(&m_threadInfo->csFrameData);						
		m_threadInfo = NULL;
	}
	DestroyWindow(m_hwndStatusDialog);
	m_hwndStatusDialog = NULL;
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
	ODS("VFWhandler::VFW_decompress()");
	VFW_CODEC_CRASH_CATCHER_START;	

	if (m_CodecPrivate.bType == PNGFrameType_RGB24) {		
		return DecompressRGBFrame(lParam1);

	} else if (m_CodecPrivate.bType == PNGFrameType_YUY2) {
		return DecompressYUY2Frame(lParam1);

	} else if (m_CodecPrivate.bType == PNGFrameType_YV12) {
		return DecompressYV12Frame(lParam1);			
	}
	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
}

int VFWhandler::DecompressRGBFrame(ICDECOMPRESS* lParam1)
{
	VFW_CODEC_CRASH_CATCHER_START;

	// Old-style RGB encoding
	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	if (m_Output_Mode == PNGOutputMode_None) {
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
	} else {
		return ICERR_UNSUPPORTED;
	}

	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
};

int VFWhandler::DecompressYUY2Frame(ICDECOMPRESS* lParam1)
{
	VFW_CODEC_CRASH_CATCHER_START;

	DWORD lActual = lParam1->lpbiInput->biSizeImage;
	
	if (m_Output_Mode == PNGOutputMode_None) {
		assert(lParam1->lpbiOutput->biCompression == FOURCC_YUY2 || lParam1->lpbiOutput->biCompression == FOURCC_YUYV);
		
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
	} else if (m_Output_Mode == PNGOutputMode_YVYU) {
		assert(lParam1->lpbiOutput->biCompression == FOURCC_YVYU);
		
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
		
		// Copy the YUV planes into the output buffer in YVYU byte order
		// Y0 V0 Y1 U0
		for (DWORD height = 0; height < Y_Channel.GetHeight()*2; height++) {
			for (DWORD width = 0; width < Y_Channel.GetWidth(); width += 4) {
				outputYUV2Data[width+0] = Y_data++[0];
				outputYUV2Data[width+1] = V_data++[0];
				outputYUV2Data[width+2] = Y_data++[0];
				outputYUV2Data[width+3] = U_data++[0];				
			}
			outputYUV2Data += width;
		}
	} else if (m_Output_Mode == PNGOutputMode_UYVY) {
		assert(lParam1->lpbiOutput->biCompression == FOURCC_UYVY);
		
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
		
		// Copy the YUV planes into the output buffer in UYVY byte order
		// U0 Y0 V0 Y1
		for (DWORD height = 0; height < Y_Channel.GetHeight()*2; height++) {
			for (DWORD width = 0; width < Y_Channel.GetWidth(); width += 4) {
				outputYUV2Data[width+0] = U_data++[0];
				outputYUV2Data[width+1] = Y_data++[0];
				outputYUV2Data[width+2] = V_data++[0];
				outputYUV2Data[width+3] = Y_data++[0];				
			}
			outputYUV2Data += width;
		}
	} else if (m_Output_Mode == PNGOutputMode_RGB24) {
		assert(lParam1->lpbiOutput->biCompression == BI_RGB);
		
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
		
		//Y_Channel.Save("D:\\Y_orig.png", CXIMAGE_FORMAT_PNG);
		//U_Channel.Save("D:\\U_orig.png", CXIMAGE_FORMAT_PNG);
		//V_Channel.Save("D:\\V_orig.png", CXIMAGE_FORMAT_PNG);

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
		// Flipping them in the process
		BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
		BYTE *U_data = (BYTE *)U_Channel.GetBits();
		BYTE *V_data = (BYTE *)V_Channel.GetBits();
		BYTE *outputYUV2Data = (BYTE *)m_DecodeBuffer.GetBuffer() + ((Y_Channel.GetHeight()-1) * Y_Channel.GetWidth() * 2);
						
		for (DWORD height = 0; height < Y_Channel.GetHeight(); height++) {
			for (DWORD width = 0; width < Y_Channel.GetWidth()*2; width += 4) {
				outputYUV2Data[width+0] = Y_data++[0];
				outputYUV2Data[width+1] = U_data++[0];
				outputYUV2Data[width+2] = Y_data++[0];
				outputYUV2Data[width+3] = V_data++[0];
				//outputYUV2Data -= 4;
			}
			outputYUV2Data -= width;
		}		

		// Using alexnoe's YUV2->RGB24 Assembly rountine
		YUV422toRGB24_MMX(m_DecodeBuffer.GetBuffer(), lParam1->lpOutput, 0, lParam1->lpbiOutput->biWidth, lParam1->lpbiOutput->biHeight, lParam1->lpbiOutput->biWidth * 2, lParam1->lpbiOutput->biWidth * 3);
	} else {
		return ICERR_UNSUPPORTED;
	}

	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
};

int VFWhandler::DecompressYV12Frame(ICDECOMPRESS* lParam1)
{
	VFW_CODEC_CRASH_CATCHER_START;
	
	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	if (m_Output_Mode == PNGOutputMode_None) {
		// Double-check the output fourcc
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
				Y_data[d] = Y_data[d] + Y_Delta_data[d];
			
			for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
				U_data[d] = U_data[d] + U_Delta_data[d];
			
			for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
				V_data[d] = V_data[d] + V_Delta_data[d];

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
	
	} else 	if (m_Output_Mode == PNGOutputMode_IYUV) {
		// Double-check the output fourcc
		assert(lParam1->lpbiOutput->biCompression == FOURCC_IYUV);

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
				Y_data[d] = Y_data[d] + Y_Delta_data[d];
			
			for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
				U_data[d] = U_data[d] + U_Delta_data[d];
			
			for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
				V_data[d] = V_data[d] + V_Delta_data[d];

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

		// The U and V planes are swapped in comparsion to YV12
		BYTE *outputYUV2Data = (BYTE *)lParam1->lpOutput;
		memcpy(outputYUV2Data, Y_Channel.GetBits(), Y_Channel.GetSizeImage());
		outputYUV2Data += Y_Channel.GetSizeImage();
		memcpy(outputYUV2Data, V_Channel.GetBits(), V_Channel.GetSizeImage());
		outputYUV2Data += V_Channel.GetSizeImage();
		memcpy(outputYUV2Data, U_Channel.GetBits(), U_Channel.GetSizeImage());
				
	} else if (m_Output_Mode == PNGOutputMode_YUY2) {
		// Double-check the output fourcc
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

		if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
			// Mix the old image and the new image together
			BYTE *Y_data = (BYTE *)Y_Channel.GetBits();
			BYTE *U_data = (BYTE *)U_Channel.GetBits();
			BYTE *V_data = (BYTE *)V_Channel.GetBits();

			BYTE *Y_Delta_data = (BYTE *)Y_Channel_Delta.GetBits();
			BYTE *U_Delta_data = (BYTE *)U_Channel_Delta.GetBits();
			BYTE *V_Delta_data = (BYTE *)V_Channel_Delta.GetBits();

			for (DWORD d = 0; d < Y_Channel.GetSizeImage(); d++)
				Y_data[d] = Y_data[d] + Y_Delta_data[d];
			
			for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
				U_data[d] = U_data[d] + U_Delta_data[d];
			
			for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
				V_data[d] = V_data[d] + V_Delta_data[d];

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

		if (m_cpu.DoesCPUSupportFeature(HAS_SSEMMX)) {
			// Call Klaus Post's isse (Integer SSE) YV12->YUY2 conversion routine
			isse_yv12_to_yuy2(
				Y_Channel.GetBits(),
				V_Channel.GetBits(), 
				U_Channel.GetBits(), 
				Y_Channel.GetWidth(), // rowsize
				Y_Channel.GetWidth(), // pitch
				Y_Channel.GetWidth()/2, // pitch_uv
				(BYTE *)lParam1->lpOutput, 
				Y_Channel.GetWidth()*2, // dst_pitch
				Y_Channel.GetHeight());
		} else {
			// Call Klaus Post's mmx YV12->YUY2 conversion routine
			mmx_yv12_to_yuy2(
				Y_Channel.GetBits(),
				V_Channel.GetBits(), 
				U_Channel.GetBits(), 
				Y_Channel.GetWidth(), // rowsize
				Y_Channel.GetWidth(), // pitch
				Y_Channel.GetWidth()/2, // pitch_uv
				(BYTE *)lParam1->lpOutput, 
				Y_Channel.GetWidth()*2, // dst_pitch
				Y_Channel.GetHeight());
		}
	} else if (m_Output_Mode == PNGOutputMode_RGB24) {
		// Double-check the output fourcc
		assert(lParam1->lpbiOutput->biCompression == BI_RGB);

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
				Y_data[d] = Y_data[d] + Y_Delta_data[d];
			
			for (DWORD d = 0; d < U_Channel.GetSizeImage(); d++)
				U_data[d] = U_data[d] + U_Delta_data[d];
			
			for (DWORD d = 0; d < V_Channel.GetSizeImage(); d++)
				V_data[d] = U_data[d] + V_Delta_data[d];

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

		//Y_Channel.Save("D:\\Y_orig_yv12.png", CXIMAGE_FORMAT_PNG);
		//U_Channel.Save("D:\\U_orig_yv12.png", CXIMAGE_FORMAT_PNG);
		//V_Channel.Save("D:\\V_orig_yv12.png", CXIMAGE_FORMAT_PNG);
		
		if (m_cpu.DoesCPUSupportFeature(HAS_SSEMMX)) {
			// Call Klaus Post's isse (Integer SSE) YV12->YUY2 conversion routine
			isse_yv12_to_yuy2(
				Y_Channel.GetBits(),
				V_Channel.GetBits(), 
				U_Channel.GetBits(), 
				Y_Channel.GetWidth(), // rowsize
				Y_Channel.GetWidth(), // pitch
				Y_Channel.GetWidth()/2, // pitch_uv
				(BYTE *)lParam1->lpOutput, 
				Y_Channel.GetWidth()*2, // dst_pitch
				Y_Channel.GetHeight());
		} else {
			// Call Klaus Post's mmx YV12->YUY2 conversion routine
			mmx_yv12_to_yuy2(
				Y_Channel.GetBits(),
				V_Channel.GetBits(), 
				U_Channel.GetBits(), 
				Y_Channel.GetWidth(), // rowsize
				Y_Channel.GetWidth(), // pitch
				Y_Channel.GetWidth()/2, // pitch_uv
				(BYTE *)lParam1->lpOutput, 
				Y_Channel.GetWidth()*2, // dst_pitch
				Y_Channel.GetHeight());
		}

		// Using alexnoe's YUV2->RGB24 Assembly rountine
		YUV422toRGB24_MMX(lParam1->lpOutput, m_DecodeBuffer.GetBuffer(), 0, lParam1->lpbiOutput->biWidth, lParam1->lpbiOutput->biHeight, lParam1->lpbiOutput->biWidth * 2, lParam1->lpbiOutput->biWidth * 3);		
		
		// Flip the image
		DWORD dwEffWidth = lParam1->lpbiOutput->biWidth * 3;
		BYTE *iSrc = (BYTE *)m_DecodeBuffer.GetBuffer() + ((lParam1->lpbiOutput->biHeight-1)*dwEffWidth);
		BYTE *iDst = (BYTE *)lParam1->lpOutput;
		for (DWORD y = 0; y < lParam1->lpbiOutput->biHeight; y++) {
			memcpy(iDst, iSrc, dwEffWidth);
			iSrc -= dwEffWidth;
			iDst += dwEffWidth;
		}		
		
	} else {
		return ICERR_UNSUPPORTED;
	}
	return ICERR_OK;
	VFW_CODEC_CRASH_CATCHER_END;
};
/******************************************************************************
* VFW_decompress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
* 
* Checks if we can decompress the incoming format. Since we can only 
* decompress our own format, we just check for that fourcc.
*
******************************************************************************/

int VFWhandler::VFW_decompress_query(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{	
	ODS("VFWhandler::VFW_decompress_query()");
	VFW_CODEC_CRASH_CATCHER_START;
	if (input->biCompression == FOURCC_PNG || input->biCompression == FOURCC_PNG_OLD)
	{
		if (output != NULL) {
#ifdef _DEBUG	
			char txt_buffer[1024];
			wsprintf(txt_buffer, "In FOURCC: %s Out FOURCC: %s", &input->biCompression, &output->biCompression);
			OutputDebugString(txt_buffer);
#endif
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
				// Check for our prefered format first
				if (output->biCompression == FOURCC_YUY2 || output->biCompression == FOURCC_YUYV) {
					m_Output_Mode = PNGOutputMode_None;
					return ICERR_OK;					
				}
				if (output->biCompression == BI_RGB && output->biBitCount == 24) {
					// Final output needs to be in RGB24
					m_Output_Mode = PNGOutputMode_RGB24;
					return ICERR_OK;
				}				
				if (output->biCompression == FOURCC_YVYU) {
					// Final output needs a different byte order
					m_Output_Mode = PNGOutputMode_YVYU;
					return ICERR_OK;					
				}
				if (output->biCompression == FOURCC_UYVY) {
					// Final output needs a different byte order
					m_Output_Mode = PNGOutputMode_UYVY;
					return ICERR_OK;					
				}								
				// No good format found
				return ICERR_BADFORMAT;

			} else if (type == PNGFrameType_YV12) {
				if (output->biCompression == FOURCC_IYUV && output->biBitCount == 12) {
					// Final output needs a different plane order
					m_Output_Mode = PNGOutputMode_IYUV;
					return ICERR_OK;
				}
				if (output->biCompression == FOURCC_YUY2 && output->biBitCount == 16) {
					// Final output needs to be converted to YUY2
					m_Output_Mode = PNGOutputMode_YUY2;
					return ICERR_OK;
				}
				if (output->biCompression == BI_RGB && output->biBitCount == 24) {
					// Final output needs to be in RGB24
					m_Output_Mode = PNGOutputMode_RGB24;
					return ICERR_OK;
				}
				
				if (output->biCompression != FOURCC_YV12) {
					//output->biCompression = FOURCC_YV12;
					return ICERR_BADFORMAT;
				}
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
	ODS("VFWhandler::VFW_decompress_get_format()");
	VFW_CODEC_CRASH_CATCHER_START;
	if (input->biCompression == FOURCC_PNG || input->biCompression == FOURCC_PNG_OLD)
	{
		if (output != NULL) {		
			output->biSize = input->biSize;
			output->biWidth = input->biWidth;
			output->biHeight = input->biHeight;
			output->biPlanes = input->biPlanes;
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
	}
	return ICERR_BADFORMAT;
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
	ODS("VFWhandler::VFW_decompress_begin()");
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
		m_Output_Mode = PNGOutputMode_None;

	} else if (m_CodecPrivate.bType == PNGFrameType_YUY2) {
		if (output->biCompression == FOURCC_YUY2) {
			// Normal output
			m_Output_Mode = PNGOutputMode_None;
		} else if (output->biCompression == BI_RGB && output->biBitCount == 24) {
			// Final output needs to be in RGB24
			m_Output_Mode = PNGOutputMode_RGB24;
			// Prep our temporay decode buffer
			m_DecodeBuffer.Open();
			m_DecodeBuffer.Seek(abs(output->biHeight) * output->biWidth * 3, 0);
			m_DecodeBuffer.Write(" ", 1, 1);
			m_DecodeBuffer.Seek(0, 0);
		} else if (output->biCompression == FOURCC_YVYU) {
			// Final output needs a different byte order
			m_Output_Mode = PNGOutputMode_YVYU;			
		} else if (output->biCompression == FOURCC_UYVY) {
			// Final output needs a different byte order
			m_Output_Mode = PNGOutputMode_UYVY;		
		} else {
			return ICERR_BADFORMAT;
		}

	} else if (m_CodecPrivate.bType == PNGFrameType_YV12) {
		if (output->biCompression == FOURCC_YV12) {
			// Normal output
			m_Output_Mode = PNGOutputMode_None;
		} else if (output->biCompression == BI_RGB && output->biBitCount == 24) {
			// Final output needs to be in RGB24
			m_Output_Mode = PNGOutputMode_RGB24;	
			// Prep our temporay decode buffer
			m_DecodeBuffer.Open();
			m_DecodeBuffer.Seek(abs(output->biHeight) * output->biWidth * 3, 0);
			m_DecodeBuffer.Write(" ", 1, 1);
			m_DecodeBuffer.Seek(0, 0);
		} else if (output->biCompression == FOURCC_IYUV && output->biBitCount == 12) {
			// Final output needs the U and V planes swapped from YV12
			m_Output_Mode = PNGOutputMode_IYUV;			
		} else if (output->biCompression == FOURCC_YUY2 && output->biBitCount == 16) {
			// Final output needs to be converted to YUY2
			m_Output_Mode = PNGOutputMode_YUY2;					
			// Prep our temporay decode buffer
			m_DecodeBuffer.Open();
			m_DecodeBuffer.Seek(abs(output->biHeight) * output->biWidth * 3, 0);
			m_DecodeBuffer.Write(" ", 1, 1);
			m_DecodeBuffer.Seek(0, 0);			
		} else {
			return ICERR_BADFORMAT;
		}
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
	ODS("VFWhandler::VFW_decompress_end()");
	// Free our temporay decode buffer
	m_DecodeBuffer.Close();

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

BOOL VFWhandler::AboutDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
  switch (uMsg)
  {
		case WM_INITDIALOG:
		{
			if (!myLogo.IsValid()) {
				HRSRC hres = FindResource(g_hInst, MAKEINTRESOURCE(IDR_PNG_LOGO), "PNG");
				HGLOBAL hgImage = LoadResource(g_hInst, hres);
				myLogo.Decode((BYTE *)LockResource(hgImage), SizeofResource(g_hInst, hres), CXIMAGE_FORMAT_PNG);
#ifdef _DEBUG
				RGBQUAD blackColor = {0, 0, 0, 0};
				myLogo.DrawTextOnImage(GetDC(hwndDlg), 244, 20, "Debug Build", blackColor, "Arial", 18, 10000);
#endif
			}
			// Increase the 'reference count'
			myLogo.SetFlags(myLogo.GetFlags() + 1);

			SetDlgItemText(hwndDlg, IDC_EDIT_ABOUT_TEXT, 
				"CorePNG, by Jory Stone <vbman@toughguy.net>" "\r\n"  "\r\n"
				
				"Orignally a test codec for image subtitles. But tests with "
				"cartoons and CGI showed very good compression ratios. "
				"When I added delta-frames amazing compression ratios resulted."
				"\r\n" "\r\n"

				"Compile Timestamp: " __DATE__ " " __TIME__ "\r\n" "\r\n"
				"YUV2->RGB24/32 ASM Routines by alexnoe" "\r\n"
				"RGB->YUY2, YUV2->YV12, YV12->YUY2 ASM Routines by Klaus Post" "\r\n"
				"A modifed version of CxImage 5.71" "\r\n"
			  "LibPNG 1.2.4" "\r\n"
				"zlib 1.1.4" "\r\n" "\r\n" 
				"And once again, thanks to Pamel for suggesting the impossible ;)"
			);
			SetFocus(GetDlgItem(hwndDlg, IDC_BUTTON_ABOUT_OK));
			break;
		}		
		case WM_PAINT:
		{			
			PAINTSTRUCT structPaint;
			BeginPaint(hwndDlg, &structPaint);
			myLogo.Draw(structPaint.hdc);		
			EndPaint(hwndDlg, &structPaint);
			break;
		}
		case WM_DESTROY:
		{	
			// I use the flags as a ref count
			myLogo.SetFlags(myLogo.GetFlags() - 1);
			if (myLogo.GetFlags() == 0)
				myLogo.Destroy();

			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		case WM_COMMAND:
			// Process button
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_ABOUT_OK:
					EndDialog(hwndDlg, IDOK);
					break;
			}
			break;
		default:
			break;
			//Nothing for now
	}
  return FALSE;
}

BOOL VFWhandler::ConfigurationDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HWND hwndToolTip = NULL;
  switch (uMsg)
  {
		case WM_INITDIALOG:
		{
			INITCOMMONCONTROLSEX common;			
			common.dwICC = ICC_UPDOWN_CLASS|ICC_WIN95_CLASSES; 
			common.dwSize = sizeof(common);
			InitCommonControlsEx(&common);

			// Create the tooltip control
			DWORD balloonTips = 0;
			if (CorePNG_GetRegistryValue("Use Balloon Tooltips", 1))
				balloonTips = TTS_BALLOON;

			hwndToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | balloonTips,
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							hwndDlg, NULL, g_hInst, NULL);
	
			// Have it cover the whole window
			SetWindowPos(hwndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			// Set our own delays
			SendMessage(hwndToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_INITIAL, (LPARAM)50);
			SendMessage(hwndToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_AUTOPOP, (LPARAM)30*1000);
			SendMessage(hwndToolTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)160);			

			if (!myLogo.IsValid()) {
				HRSRC hres = FindResource(g_hInst, MAKEINTRESOURCE(IDR_PNG_LOGO), "PNG");
				HGLOBAL hgImage = LoadResource(g_hInst, hres);
				myLogo.Decode((BYTE *)LockResource(hgImage), SizeofResource(g_hInst, hres), CXIMAGE_FORMAT_PNG);
#ifdef _DEBUG
				RGBQUAD blackColor = {0, 0, 0, 0};
				myLogo.DrawTextOnImage(GetDC(hwndDlg), 244, 20, "Debug Build", blackColor, "Arial", 18, 10000);
#endif
			}
			// Increase the 'reference count'
			myLogo.SetFlags(myLogo.GetFlags() + 1);
			RECT dialogImage = { 0, 0, 350, 100 };
			InvalidateRect(hwndDlg, &dialogImage, true);

			CheckDlgButton(hwndDlg, IDC_CHECK_ENABLE_STATUS_DIALOG, m_EnableStatusDialog);
			CheckDlgButton(hwndDlg, IDC_CHECK_CRASH_CATCHER, CorePNG_GetRegistryValue("Crash Catcher Enabled", 0));
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
			
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL), "Max number of delta-frames to encode in a row.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_EDIT_KEYFRAME_INTERVAL), "Max number of delta-frames to encode in a row.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_CHECK_DELTA_FRAMES), "Enable Delta frames");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_CHECK_AUTO_DELTA_FRAMES), "Use Auto-Delta frames, when encoding delta-frames are encoded twice. Once as keyframes and another time as delta frames, the smaller frame is used.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_COMBO_COMPRESSION_LEVEL), "Set your compression level here.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_CHECK_DECODE_RGB24), "Output 32-bit RGB as 24-bit RGB, striping the Alpha channel.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_CHECK_CRASH_CATCHER), "Enable crash catcher. Use this to send me bug reports that are directly related to CorePNG. It is disabled by default because it can/will catch other program crashes too. This may need a program restart to take effect.");
			AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_CHECK_ENABLE_STATUS_DIALOG), "Enable Status dialog while encoding. Display infomation like, the colorspace, number of keyframes and deltaframes encoded.");
			
			break;
		}		
		case WM_PAINT:
		{			
			PAINTSTRUCT structPaint;
			BeginPaint(hwndDlg, &structPaint);
			myLogo.Draw(structPaint.hdc);		
			EndPaint(hwndDlg, &structPaint);
			break;
		}
		case WM_DESTROY:
		{			
			myLogo.SetFlags(myLogo.GetFlags() - 1);
			if (myLogo.GetFlags() == 0)
				myLogo.Destroy();

			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
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
					m_EnableStatusDialog = IsDlgButtonChecked(hwndDlg, IDC_CHECK_ENABLE_STATUS_DIALOG);
					
					CorePNG_SetRegistryValue("Crash Catcher Enabled", IsDlgButtonChecked(hwndDlg, IDC_CHECK_CRASH_CATCHER));

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
				case IDC_BUTTON_DISPLAY_ABOUT:
					this->VFW_about(hwndDlg);
					break;
			}
			break;
		default:
			break;
			//Nothing for now
	}
  return FALSE;
}

BOOL VFWhandler::StatusDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HWND hwndToolTip = NULL;
  switch (uMsg)
  {
		case WM_INITDIALOG:
		{


			// Create the tooltip control
			DWORD balloonTips = 0;
			if (CorePNG_GetRegistryValue("Use Balloon Tooltips", 1))
				balloonTips = TTS_BALLOON;

			hwndToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | balloonTips,
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							hwndDlg, NULL, g_hInst, NULL);
	
			// Have it cover the whole window
			SetWindowPos(hwndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			// Set our own delays
			SendMessage(hwndToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_INITIAL, (LPARAM)50);
			SendMessage(hwndToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_AUTOPOP, (LPARAM)30*1000);
			SendMessage(hwndToolTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)160);			

			if (!myLogo.IsValid()) {
				HRSRC hres = FindResource(g_hInst, MAKEINTRESOURCE(IDR_PNG_LOGO), "PNG");
				HGLOBAL hgImage = LoadResource(g_hInst, hres);
				myLogo.Decode((BYTE *)LockResource(hgImage), SizeofResource(g_hInst, hres), CXIMAGE_FORMAT_PNG);
#ifdef _DEBUG
				RGBQUAD blackColor = {0, 0, 0, 0};
				myLogo.DrawTextOnImage(GetDC(hwndDlg), 244, 20, "Debug Build", blackColor, "Arial", 18, 10000);
#endif
			}
			// Increase the 'reference count'
			myLogo.SetFlags(myLogo.GetFlags() + 1);
			RECT dialogImage = { 0, 0, 350, 100 };
			InvalidateRect(hwndDlg, &dialogImage, true);

			std::string encodingMode = "Encoding Colorspace: ";
			if (m_Input_Mode == BI_RGB) {				
				encodingMode += "RGB";
			} else if (m_Input_Mode == FOURCC_YUY2) {				
				encodingMode += "YUY2";
			} else if (m_Input_Mode == FOURCC_YV12) {				
				encodingMode += "YV12";
			} else {				
				encodingMode += "Error";
			}
			SetDlgItemText(hwndDlg, IDC_STATIC_ENCODING_MODE, encodingMode.c_str());
			//AddTooltip(hwndToolTip, GetDlgItem(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL), "Max number of delta-frames to encode in a row.");

			// Check for Win2000
			OSVERSIONINFO osVersion;
			osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osVersion);
			if (osVersion.dwMajorVersion >= 5) {
				SendDlgItemMessage(hwndDlg, IDC_SLIDER_TRANSPARENT, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM) MAKELONG(10, 100));
				SendDlgItemMessage(hwndDlg, IDC_SLIDER_TRANSPARENT, TBM_SETPOS, TRUE, CorePNG_GetRegistryValue("Status Dialog Transparency", 100));

				StatusDlgProc(hwndDlg, WM_HSCROLL, 0, (LPARAM)GetDlgItem(hwndDlg, IDC_SLIDER_TRANSPARENT));
			} else {
				ShowWindow(GetDlgItem(hwndDlg, IDC_STATIC_TRANSPARENT), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_SLIDER_TRANSPARENT), SW_HIDE);
			}
			SetTimer(hwndDlg, 555, 500, NULL);
			break;
		}		
		case WM_PAINT:
		{			
			PAINTSTRUCT structPaint;
			BeginPaint(hwndDlg, &structPaint);
			myLogo.Draw(structPaint.hdc);		
			EndPaint(hwndDlg, &structPaint);
			break;
		}
		case WM_DESTROY:
		{			
			myLogo.SetFlags(myLogo.GetFlags() - 1);
			if (myLogo.GetFlags() == 0)
				myLogo.Destroy();

			KillTimer(hwndDlg, 555);
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		case WM_HSCROLL:
		{
			BYTE bPos = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			CorePNG_SetRegistryValue("Status Dialog Transparency", bPos);
			if ((bPos == 100) && !(GetWindowLong(hwndDlg, GWL_EXSTYLE) & GWL_EXSTYLE)) {
				// Remove WS_EX_LAYERED from this window styles
				SetWindowLong(hwndDlg, GWL_EXSTYLE, GetWindowLong(hwndDlg, GWL_EXSTYLE) & ~WS_EX_LAYERED);
				// Ask the window and its children to repaint
				RedrawWindow(hwndDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			} else {
				if (GetWindowLong(hwndDlg, GWL_EXSTYLE) & GWL_EXSTYLE) {
					// Set WS_EX_LAYERED on this window 
					SetWindowLong(hwndDlg, GWL_EXSTYLE, GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
				}
				static SetLayeredWindowAttributes_def pSetLayeredWindowAttributes = (SetLayeredWindowAttributes_def)GetProcAddress(LoadLibrary("User32.dll"), "SetLayeredWindowAttributes");
				if (pSetLayeredWindowAttributes != NULL)
					pSetLayeredWindowAttributes(hwndDlg, 0, (255 * bPos) / 100, LWA_ALPHA);
			}
			break;
		}
		case WM_TIMER:
		{
			if (wParam == 555) {
				std::string txt_bufferStr;
				char txt_buffer[1025];
				wsprintf(txt_buffer, " Keyframes:\t %i \n Delta-frames:\t %i", m_TotalKeyframes, m_TotalDeltaframes);
				txt_bufferStr = txt_buffer;
				GetDlgItemText(hwndDlg, IDC_STATIC_STATUS_FRAMES, txt_buffer, 1024);
				if (txt_bufferStr.compare(txt_buffer)) {				
					SetDlgItemText(hwndDlg, IDC_STATIC_STATUS_FRAMES, txt_bufferStr.c_str());
				}
			}
			break;
		}
		case WM_COMMAND:
			// Process button
			switch (LOWORD(wParam))
			{
				
				case IDC_BUTTON_DISPLAY_ABOUT:
					this->VFW_about(hwndDlg);
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

BOOL CALLBACK AboutDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	VFWhandler *vfwData = (VFWhandler *)GetWindowLong(hwndDlg, DWL_USER);	
  if (uMsg == WM_INITDIALOG) {
		// Store the VFWhandler
		vfwData = (VFWhandler *)lParam;
		SetWindowLong(hwndDlg, DWL_USER, (LONG)vfwData);
	}
	if (vfwData != NULL) {
		return vfwData->AboutDlgProc(hwndDlg, uMsg, wParam, lParam);
	}
	return FALSE;
};

BOOL CALLBACK StatusDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	VFWhandler *vfwData = (VFWhandler *)GetWindowLong(hwndDlg, DWL_USER);	
  if (uMsg == WM_INITDIALOG) {
		// Store the VFWhandler
		vfwData = (VFWhandler *)lParam;
		SetWindowLong(hwndDlg, DWL_USER, (LONG)vfwData);
	}
	if (vfwData != NULL) {
		return vfwData->StatusDlgProc(hwndDlg, uMsg, wParam, lParam);
	}
	return FALSE;
};

WORD AddTooltip(HWND hwndTooltip, HWND hwndClient, LPSTR strText)
{
	static WORD id = 0;
	TOOLINFO ti;  
	RECT rect;

	// Get the coordinates of the client control
	GetClientRect(hwndClient, &rect);
	
  // Setup the toolinfo structure
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = hwndClient;
	ti.hinst = g_hInst;
	ti.uId = id;
	ti.lpszText = strText;

	// Tooltip control will cover the whole window
	ti.rect.left	= rect.left;    
	ti.rect.top		= rect.top;
	ti.rect.right	= rect.right;
	ti.rect.bottom	= rect.bottom;
	
	// Add the tooltip
	SendMessage(hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);	

	// Return the id of the tooltip
	return id++;
} 

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

#ifdef MY_OWN_COLORSPACE_CONVERSION
void YV12toRGB24(BYTE *pInput, BYTE *pOutput, DWORD dwWidth, DWORD dwHeight)
{
/*
	YV12 to RGB24 conversion routine
	; YUY 16-235 -> RGB
	; D:= Y - 16
	; E= U - 128
	; F:= V - 128
	; B:= 1.164*D + 2.018*E
	; G:= 1.164*D - 0.391*E - 0.813*F 
	; R:= 1.164*D           - 1.596*F
*/	
	/*
	BYTE* lpYV12Src = pInput;
	BYTE* lpYV12SrcY = pInput + (dwHeight * dwWidth);
	BYTE* lpYV12SrcU = pInput + (dwHeight/2 * dwWidth/2);
	BYTE* lpYV12SrcV = pInput + (dwHeight/2 * dwWidth/2);
	RGBTRIPLE* lprgbDst = (RGBTRIPLE *)pOutput;

	for (DWORD y = 0; y < dwHeight; y++) {
		for (DWORD x = 0; x < dwWidth; x++) {			
			BYTE D = lpYV12SrcY[x] - 16;
			BYTE E = lpYV12SrcU[x] - 128;
			BYTE F = lpYV12SrcV[x] - 128;
			lprgbDst[x].rgbtBlue = 1.164*D + 2.018*E;
			lprgbDst[x].rgbtGreen = 1.164*D - 0.391*E - 0.813*F;
			lprgbDst[x].rgbtRed = 1.164*D - 1.596*F;
		}
		// Move to next scan line.
		//lpYV12Src = ((LPBYTE)lpYV12Src + dwWidthBytes);		
		lprgbDst  = (RGBTRIPLE *)((LPBYTE)lprgbDst  + dwWidthBytes);
	}
	*/
};

void YV12toYUY2(BYTE *pInput, BYTE *pOutput, DWORD dwWidth, DWORD dwHeight)
{	
	DWORD dwEffWidth = dwWidth*4;
	BYTE *y_plane = pInput;
	BYTE *u_plane = pInput + (dwHeight/4 * dwWidth/4);
	BYTE *v_plane = pInput + (dwHeight/4 * dwWidth/4);
	BYTE *firstScanLine = pOutput;
	BYTE *secondScanLine = pOutput+dwEffWidth;

#ifdef _DEBUG
	// Set the whole plane to black
	memset(pOutput, 0, dwEffWidth*dwHeight);
#endif
	// We do two rows at a time
	for (DWORD y = 0; y < dwHeight*2; y++) {
		for (DWORD x = 0; x < dwWidth; x+=4) {
			//firstScanLine[x] = 235;
			//firstScanLine[x+2] = 235;
			//secondScanLine[x] = 235;
			
			//firstScanLine[x+2] = 235;
			//secondScanLine[x+2] = 235;
			firstScanLine[x] = y_plane[x+0];
			secondScanLine[x] = y_plane[x+0];		
						
			firstScanLine[x+1] = u_plane[0];			
			secondScanLine[x+1] = u_plane++[0];

			firstScanLine[x+3] = v_plane[0];
			secondScanLine[x+3] = v_plane++[0];
							
			firstScanLine[x+2] = y_plane[x+1];
			secondScanLine[x+2] = y_plane[x+1];			
		}
		firstScanLine += dwEffWidth*2;
		secondScanLine += dwEffWidth*2;
	}
};

void YV12toRGB24_Resample(CxImage &Y_plane, CxImage &U_plane, CxImage &V_plane, BYTE *pOutput, WORD wMethod)
{
/*
	YV12 to RGB24 conversion routine
	; YUY 16-235 -> RGB
	; D:= Y - 16
	; E= U - 128
	; F:= V - 128
	; B:= 1.164*D + 2.018*E
	; G:= 1.164*D - 0.391*E - 0.813*F 
	; R:= 1.164*D           - 1.596*F
*/	
	// Upsample the UV channels
	U_plane.Resample(Y_plane.GetWidth(), Y_plane.GetHeight(), 1);
	V_plane.Resample(Y_plane.GetWidth(), Y_plane.GetHeight(), 1);

	//U_plane.Negative();
	//V_plane.Negative();
	//Y_plane.Draw(GetDC(NULL));
	//U_plane.Draw(GetDC(NULL));
	//V_plane.Draw(GetDC(NULL));

	RGBTRIPLE* lprgbDst = (RGBTRIPLE *)pOutput;
	DWORD dwWidthBytes = Y_plane.GetWidth() * 3;

	for (DWORD y = 0; y < Y_plane.GetHeight(); y++) {
		for (DWORD x = 0; x < Y_plane.GetWidth(); x++) {			
			BYTE D = Y_plane.GetPixelGray(x, y) - 16;
			BYTE E = U_plane.GetPixelGray(x, y) - 128;
			BYTE F = V_plane.GetPixelGray(x, y) - 128;
			lprgbDst[x].rgbtBlue = 1.164*D + 2.018*E;
			lprgbDst[x].rgbtGreen = 1.164*D - 0.391*E - 0.813*F;
			lprgbDst[x].rgbtRed = 1.164*D - 1.596*F;
		}
		// Move to next scan line.
		//lpYV12Src = ((LPBYTE)lpYV12Src + dwWidthBytes);		
		lprgbDst = (RGBTRIPLE *)((LPBYTE)lprgbDst + dwWidthBytes);
	}
};
#endif
