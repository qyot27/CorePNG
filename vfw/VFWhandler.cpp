/******************************************************************************
* VFWhandler.cpp
*
* For CorePNG Codec Video For Windows interface
* by Jory Stone 
* based on WARP VFW interface by General Lee D. Mented (gldm@mail.com)
*
******************************************************************************/

#include "VFWhandler.h"

VFWhandler::VFWhandler(void)
{
	m_Memfile.Open();
}

VFWhandler::~VFWhandler(void)
{

}

/******************************************************************************
* VFW_Configure();
*
* This basicly pops up the dialog box to configure codec settings. It will
* store the settings in the WARPconfig myconfig (see below) and then
* pass them to the codec during VFW_compress_begin.
*
******************************************************************************/

void VFWhandler::VFW_configure()
{
	int configcode;
	configcode = 0;
}

/******************************************************************************
* VFW_compress(ICCOMPRESS* lParam1, DWORD lParam2);
*
* This function is used to compress individual frames during compression.
*
******************************************************************************/

int VFWhandler::VFW_compress(ICCOMPRESS* lParam1, DWORD lParam2)
{
	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	// Now compress the frame.
	if (lParam1->lpbiInput->biBitCount == 24) {
		memcpy(m_Image.GetBits(), lParam1->lpInput, lActual);
	} else if (lParam1->lpbiInput->biBitCount == 32) {
		m_Image.CreateFromARGB(lParam1->lpbiInput->biWidth, lParam1->lpbiInput->biWidth, (BYTE *)lParam1->lpInput);
	}
	m_Memfile.Seek(0, 0);		
	m_Image.Encode(&m_Memfile, CXIMAGE_FORMAT_PNG);	
	//m_Image.Draw(GetDC(NULL));

	lActual = m_Memfile.Tell();	

	// Now put the result back
	memcpy(lParam1->lpOutput, m_Memfile.GetBuffer(), lActual);
	lParam1->lpbiOutput->biSizeImage = lActual;
	lParam1->lpbiOutput->biSize = sizeof(BITMAPINFOHEADER);
	lParam1->lpbiOutput->biWidth = lParam1->lpbiInput->biWidth;
	lParam1->lpbiOutput->biHeight = lParam1->lpbiInput->biWidth;
	lParam1->lpbiOutput->biCompression = FOURCC_PNG;
	lParam1->lpbiOutput->biClrUsed = 0;
	lParam1->dwFlags = ICCOMPRESS_KEYFRAME;
	*lParam1->lpdwFlags = AVIIF_KEYFRAME;
	return ICERR_OK;
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
	if(input->biCompression == 0 && (input->biBitCount == 24 || input->biBitCount == 32))
	{
		return ICERR_OK;
	}
	else
	{
		return ICERR_BADFORMAT;
	}
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
	if(output == NULL)
	{
		return sizeof(BITMAPINFOHEADER);
	}

		output->biSizeImage = input->biSizeImage;
		output->biSize = input->biSize;
		output->biWidth = input->biWidth;
		output->biHeight = input->biHeight;
		output->biCompression = FOURCC_PNG;
		output->biClrUsed = 0;
		output->biBitCount = input->biBitCount;

	return ICERR_OK;
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
	return (input->biHeight * input->biWidth * (input->biBitCount/8));
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
	m_Image.Create(input->biWidth, input->biHeight, 24);
	//mycodec->Configure(*myconfig);
	return ICERR_OK;
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
	DWORD lActual = lParam1->lpbiInput->biSizeImage;

	// Now decompress the frame.
	m_Image.Decode((BYTE *)lParam1->lpInput, lActual, CXIMAGE_FORMAT_PNG);		

	lParam1->lpbiOutput->biSizeImage = lParam1->lpbiInput->biWidth * lParam1->lpbiInput->biHeight * (lParam1->lpbiInput->biBitCount / 8);

	if (lParam1->lpbiOutput->biBitCount == 32) {
		// Convert the 24-bit+Alpha to 32-bits
		m_Image.Flip();
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
	lParam1->lpbiOutput->biSize = sizeof(BITMAPINFOHEADER);
	lParam1->lpbiOutput->biWidth = lParam1->lpbiInput->biWidth;
	lParam1->lpbiOutput->biHeight = lParam1->lpbiInput->biHeight;
	lParam1->lpbiOutput->biCompression = FOURCC_PNG;
	lParam1->lpbiOutput->biClrUsed = 0;

	return ICERR_OK;
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
	if (input->biCompression == FOURCC_PNG || input->biCompression == FOURCC_PNG_OLD)
	{
		if (output != NULL) {
			if (output->biCompression != BI_RGB)
				// Not RGB
				return ICERR_BADFORMAT;
			if (output->biBitCount != 24 && output->biBitCount != 32)
				// Bad bit-depth
				return ICERR_BADFORMAT;
		}
		return ICERR_OK;
	}
	else
	{
		return ICERR_BADFORMAT;
	}
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
	if (output != NULL) {
		output->biSizeImage = input->biSizeImage;
		output->biSize = input->biSize;
		output->biWidth = input->biWidth;
		output->biHeight = input->biHeight;
		output->biCompression = BI_RGB;
		output->biBitCount = input->biBitCount;
	}
	return ICERR_OK;
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
	m_Image.Create(input->biWidth, input->biHeight, 24);
	//mycodec->Configure(*myconfig);
	return ICERR_OK;
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
