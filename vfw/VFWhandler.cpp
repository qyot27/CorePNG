/******************************************************************************
* VFWhandler.cpp
*
* For CorePNG Codec Video For Windows interface
* by Jory Stone 
* based on WARP VFW interface by General Lee D. Mented (gldm@mail.com)
*
******************************************************************************/

#include "VFWhandler.h"

extern HINSTANCE g_hInst;

VFWhandler::VFWhandler(void)
{
	m_BufferSize = 0;
	m_DeltaFrameCount = 0;
	LoadSettings();
}

VFWhandler::~VFWhandler(void)
{
	//SaveSettings();
}

void VFWhandler::LoadSettings() {
	m_DeltaFrameLimit = CorePNG_GetRegistryValue("Keyframe Interval", 2);
	m_ZlibCompressionLevel = CorePNG_GetRegistryValue("zlib Compression Level", 6);
	m_PNGFilters = CorePNG_GetRegistryValue("PNG Filters", PNG_ALL_FILTERS);
	m_DeltaFramesEnabled = CorePNG_GetRegistryValue("Use Delta Frames", 0);
	m_DecodeToRGB24 = CorePNG_GetRegistryValue("Always Decode to RGB24", 0);
	m_DropFrameThreshold = CorePNG_GetRegistryValue("Drop Frame Threshold", 0);
}

void VFWhandler::SaveSettings() {
	CorePNG_SetRegistryValue("Keyframe Interval", m_DeltaFrameLimit);
	CorePNG_SetRegistryValue("zlib Compression Level", m_ZlibCompressionLevel);
	CorePNG_SetRegistryValue("PNG Filters", m_PNGFilters);
	CorePNG_SetRegistryValue("Use Delta Frames", m_DeltaFramesEnabled);
	CorePNG_SetRegistryValue("Always Decode to RGB24", m_DecodeToRGB24);
	CorePNG_SetRegistryValue("Drop Frame Threshold", m_DropFrameThreshold);
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

	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	if (!m_DeltaFrameCount) {
		// Now compress the frame.
		if (lParam1->lpbiInput->biBitCount == 24) {
			if (m_DropFrameThreshold != 0) {
				DWORD imageDiff = abs(memcmp(m_Image.GetBits(), lParam1->lpInput, lActual));
				if (imageDiff < m_DropFrameThreshold) {
					// Drop this frame
					lActual = 0;
				}
			}
			memcpy(m_Image.GetBits(), lParam1->lpInput, lActual);
			if (m_DeltaFramesEnabled) {
				m_DeltaFrame = m_Image;
				m_DeltaFrameCount++;
			}
		} else if (lParam1->lpbiInput->biBitCount == 32) {
			m_Image.CreateFromARGB(lParam1->lpbiInput->biWidth, lParam1->lpbiInput->biHeight, (BYTE *)lParam1->lpInput);
			m_Image.Flip();
			/*
			for (DWORD height = 0; height < m_Image.GetHeight(); height++) {
				for (DWORD width = 0; width < m_Image.GetWidth(); width++) {
					if (m_Image.AlphaGet(width, height) != 0)
						lActual = lActual;
				}
			}
			*/
		}
		if (lActual != 0) {
			CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
			m_Image.Encode(&memFile);	
			//m_Image.Draw(GetDC(NULL));

			lActual = memFile.Tell();	
		}
		// Now put the result back	
		lParam1->lpbiOutput->biSizeImage = lActual;
		//lParam1->lpbiOutput->biCompression = FOURCC_PNG;
		lParam1->dwFlags = ICCOMPRESS_KEYFRAME;
		*lParam1->lpdwFlags = AVIIF_KEYFRAME;
	} else {
		// We compress a delta frame
		if (lParam1->lpbiInput->biBitCount == 24) {
			memcpy(m_Image.GetBits(), lParam1->lpInput, lActual);
			if (!m_Image.AlphaIsValid())
				m_Image.AlphaCreate();
			else
				m_Image.AlphaClear();
			
			// Compare to the previous frame
			RGBQUAD previousPixel;
			RGBQUAD currentPixel;
			//m_Image.Draw(GetDC(NULL));
			//m_DeltaFrame.Draw(GetDC(NULL));
			m_Image.Mix(m_DeltaFrame, CxImage::ImageOpType::OpSub2);
			//m_Image.Draw(GetDC(NULL));
			/*for (DWORD y = 0; y < m_Image.GetHeight(); y++) {
				for (DWORD x = 0; x < m_Image.GetWidth(); x++) {
					currentPixel = m_Image.GetPixelColor(x, y);
					previousPixel = m_DeltaFrame.GetPixelColor(x, y);
					if (!memcmp(&currentPixel, &previousPixel, sizeof(RGBQUAD))) {
						// Matching pixels
						// Average pixels		
						memset(&currentPixel, 0, sizeof(RGBQUAD));
						m_Image.SetPixelColor(x, y, currentPixel, true);		
						//m_Image.AlphaSet(x, y, 0);
					}
				}
			}*/
			// Replace the delta frame with the full frame
			memcpy(m_DeltaFrame.GetBits(), lParam1->lpInput, lActual);
			if (m_DeltaFrameCount++ > m_DeltaFrameLimit)
				m_DeltaFrameCount = 0;
			
		} else {
			// We only support delta-frames with 24-bit compression
			return ICERR_ERROR;
		}
		CxMemFile memFile((BYTE *)lParam1->lpOutput, m_BufferSize);
		m_Image.Encode(&memFile);	
		//m_Image.Draw(GetDC(NULL));

		lActual = memFile.Tell();	

		// Now put the result back	
		lParam1->lpbiOutput->biSizeImage = lActual;
		//lParam1->lpbiOutput->biSize = sizeof(BITMAPINFOHEADER);
		//lParam1->lpbiOutput->biWidth = lParam1->lpbiInput->biWidth;
		//lParam1->lpbiOutput->biHeight = lParam1->lpbiInput->biWidth;
		//lParam1->lpbiOutput->biCompression = FOURCC_PNG;
		//lParam1->lpbiOutput->biClrUsed = 0;
		lParam1->dwFlags = 0;
		*lParam1->lpdwFlags = 0;
	}
	return ICERR_OK;

	VFW_CODEC_CRASH_CATCHER_END;
}

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
	//if(input->biCompression == FOURCC_YUY2)
	//	return ICERR_OK;

	if(input->biCompression == 0 
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
		return sizeof(BITMAPINFOHEADER);

	output->biSizeImage = input->biSizeImage;
	output->biSize = input->biSize;
	output->biWidth = input->biWidth;
	output->biHeight = input->biHeight;
	output->biCompression = FOURCC_PNG;
	output->biClrUsed = 0;
	output->biBitCount = input->biBitCount;

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
	return m_BufferSize = (input->biHeight * input->biWidth * (input->biBitCount/8));
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

	m_Image.Create(input->biWidth, input->biHeight, 24);
	if (m_ZlibCompressionLevel == 0) {
		m_Image.SetCompressionFilters(PNG_NO_FILTERS);
	}

	m_Image.SetCompressionLevel(m_ZlibCompressionLevel);

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

	// Preserve the previous frame
	if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
		m_DeltaFrame = m_Image;
	}

	// Now decompress the frame.
	CxMemFile memFile((BYTE *)lParam1->lpInput, lActual);
	m_Image.Decode(&memFile);		
	if (lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME) {
		// Mix the old image and the new image together
		m_DeltaFrame.Mix(m_Image, CxImage::ImageOpType::OpAdd2);
		m_Image = m_DeltaFrame;
	}

	//lParam1->lpbiOutput->biSizeImage = lParam1->lpbiInput->biWidth * lParam1->lpbiInput->biHeight * (lParam1->lpbiInput->biBitCount / 8);

	if (lParam1->lpbiOutput->biBitCount == 32) {
		// Convert the 24-bit+Alpha to 32-bits
		RGBTRIPLE *decodedImage = (RGBTRIPLE *)m_Image.GetBits();	
		RGBQUAD *decodedOutput = (RGBQUAD *)lParam1->lpOutput;

		for (DWORD height = 0; height < m_Image.GetHeight(); height++) {
			for (DWORD width = 0; width < m_Image.GetWidth(); width++) {
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbBlue = decodedImage[(m_Image.GetWidth()*height)+width].rgbtBlue;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbGreen = decodedImage[(m_Image.GetWidth()*height)+width].rgbtGreen;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbRed = decodedImage[(m_Image.GetWidth()*height)+width].rgbtRed;
				decodedOutput[(m_Image.GetWidth()*height)+width].rgbReserved = m_Image.AlphaGet(width, height);
			}
		}
	} else if (lParam1->lpbiOutput->biBitCount == 24) {
		memcpy(lParam1->lpOutput, m_Image.GetBits(), lParam1->lpbiOutput->biSizeImage);
	}
/*	lParam1->lpbiOutput->biSize = sizeof(BITMAPINFOHEADER);
	lParam1->lpbiOutput->biWidth = lParam1->lpbiInput->biWidth;
	lParam1->lpbiOutput->biHeight = lParam1->lpbiInput->biHeight;
	lParam1->lpbiOutput->biCompression = FOURCC_PNG;
	lParam1->lpbiOutput->biClrUsed = 0;
*/	
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
			if (output->biCompression != BI_RGB)
				// Not RGB
				return ICERR_BADFORMAT;
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
		}
		return ICERR_OK;
	}
	else
	{
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
		output->biSizeImage = input->biSizeImage;
		output->biSize = input->biSize;
		output->biWidth = input->biWidth;
		output->biHeight = input->biHeight;
		output->biCompression = BI_RGB;
		if (m_DecodeToRGB24) {
				output->biBitCount = 24;
		} else {
			output->biBitCount = input->biBitCount;
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

			HMODULE myModule = GetModuleHandle("CorePNG_vfw.dll");	
			HRSRC hres = FindResource(myModule, MAKEINTRESOURCE(IDR_PNG_LOGO), "PNG");
			HGLOBAL hgImage = LoadResource(myModule, hres);
			myLogo.Decode((BYTE *)LockResource(hgImage), SizeofResource(myModule, hres), CXIMAGE_FORMAT_PNG);

			CheckDlgButton(hwndDlg, IDC_CHECK_DECODE_RGB24, m_DecodeToRGB24);
			CheckDlgButton(hwndDlg, IDC_CHECK_DELTA_FRAMES, m_DeltaFramesEnabled);
						
			SendDlgItemMessage(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL, UDM_SETRANGE, 0, (LPARAM)MAKELONG(1000, 1));
			SendDlgItemMessage(hwndDlg,	IDC_SPIN_KEYFRAME_INTERVAL, UDM_SETPOS, 0, m_DeltaFrameLimit+1);

			SendDlgItemMessage(hwndDlg,	IDC_SPIN_DROP_THRESHOLD, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
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
