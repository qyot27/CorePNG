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

/******************************************************************************
* Comments by Gldm
* This file is the entry point for a VFW DLL to begin executing. I really don't know
* all that much about the specifics. This was derived from xvid's driverproc.c and 
* huffyuv's driverproc.c, and converted for C++ by me. There may be bugs as I really
* don't understand what some of it does. Refer to Vfw.h and MSDN for more info.
*
* My idea here was get this part over and done with and into a sane object
* as soon as possible. Hence it basicly just returns some values and calls the
* VFWhandler object that does most of the dirty work related to VFW.
*
******************************************************************************/
#ifdef _DEBUG
//Memory Debugging define
#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <vfw.h>

#include "VFWhandler.h"

// The global dll handle
HINSTANCE g_hInst;

// This is the DllMain.
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
#ifdef _DEBUG
	// Setup the debug options
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF 
								| _CRTDBG_LEAK_CHECK_DF //Check for memory leaks on app exit
								| _CRTDBG_CHECK_ALWAYS_DF); // Check memory heaps at each new and delete call
	//_CrtSetAllocHook(YourAllocHook);
#endif

	g_hInst = (HINSTANCE) hModule;
	return TRUE;
}

// Here's the function that really matters, DriverProc.
// Pretty much all the action happens here.
extern "C" __declspec(dllexport) LRESULT WINAPI DriverProc(DWORD dwDriverId, HDRVR hDriver, UINT uMsg, LPARAM lParam1, LPARAM lParam2) 
{
	// Ok make a VFWhandler out of whatever we're given.
	VFWhandler* handler = (VFWhandler*)dwDriverId;

	// Now figure out what to do based on uMsg
	switch(uMsg)
	{

	// Some driver functions that seem to do nothing?
	// They were like this when I found them.
	case DRV_LOAD :
	case DRV_FREE :
		DRV_OK;

	// This seems to be where a new codec instance is created.
	case DRV_OPEN :
		//{
		//ICOPEN* icopen = (ICOPEN*)lParam2;
		//if(handler == 0)
			handler = new VFWhandler();
		return (LRESULT)handler;
		//}

	// Obviously if open creates, close should delete.
	case DRV_CLOSE :
		delete handler;
		handler = NULL;
		return DRV_OK;

	// These were also empty when I found them.
	case DRV_DISABLE :
	case DRV_ENABLE :
		return DRV_OK;

	// Also found empty.
	case DRV_INSTALL :
	case DRV_REMOVE :
		return DRV_OK;

	// Also empty. No idea what DRV_CANCEL does or why to use it.
	case DRV_QUERYCONFIGURE :
	case DRV_CONFIGURE :
		return DRV_CANCEL;


	// This seems to be a way to figure out what this codec does.
	case ICM_GETINFO :
		{
			ICINFO* icinfo = (ICINFO *)lParam1;

			icinfo->fccType = ICTYPE_VIDEO;
			icinfo->fccHandler = FOURCC_PNG;
			icinfo->dwFlags = VIDCF_FASTTEMPORALC | VIDCF_FASTTEMPORALD | VIDCF_TEMPORAL;
			icinfo->dwVersion = 0;
			icinfo->dwVersionICM = ICVERSION;
			
			wcscpy(icinfo->szName, L"CorePNG"); 
			wcscpy(icinfo->szDescription, COREPNG_VFW_VERSIONW);
						
			return lParam2;
		}

	// About and configure seem to have been combined by the XVID people.
	// This seems to be where your dialog box comes up to set your settings.
	// If the app calls this with -1, it just wants to know if you have a
	// configure box, so just return ok.
	case ICM_ABOUT :
		if (lParam1 != -1) {
			MessageBox((HWND)lParam1, "CorePNG was developed by Jory Stone <vbman@toughguy.net>\nAs a test codec for image subtitles. But as it can be used for other things.\nTimestamp: " __DATE__ " " __TIME__, "About CorePNG", MB_ICONINFORMATION);
		}
		return ICERR_OK;
	case ICM_CONFIGURE :
		if (lParam1 != -1) {
			handler->VFW_configure((HWND)lParam1);
		}
		return ICERR_OK;
			
	// This is used to get the configuration of the codec and store it.
	// If the first parameter is NULL, it needs to return the size
	// needed to store the config info. Otherwise the first parameter
	// is a pointer to the config info, and the 2nd is the size in bytes.
	case ICM_GETSTATE :
		if(lParam1 == NULL)
		{
			return sizeof(CorePNGCodecSettings);
		}
		else
		{
			memcpy((void*)lParam1, handler, handler->wSize);
		}
		return ICERR_OK;

	// The reverse of GETSTATE. This time if the first parameter is NULL
	// it should reset things to defaults. Otherwise the first one is
	// a pointer to a set of config info, and the second its size in bytes.
	case ICM_SETSTATE :
		if(lParam1 == NULL)
		{
			//handler->setdefaults();
		}
		else
		{
			memcpy((void*)handler, (void*)lParam1, handler->wSize);
		}
		return ICERR_OK;

	// According to MS these are used to get and set the "state"
	// of the codec. I don't really know what they want.
	case ICM_GET :
	case ICM_SET :
		return ICERR_OK;

	// All blank in the original. They're unsupported.
	case ICM_GETDEFAULTQUALITY :
	case ICM_GETQUALITY :
	case ICM_SETQUALITY :
	case ICM_GETBUFFERSWANTED :
	case ICM_GETDEFAULTKEYFRAMERATE :
		return ICERR_UNSUPPORTED;

// Ok now we come to the interesting part!
// Compression section.

		
	// The actual compression function. Wow, never thought I'd find it in all this gibberish.
	case ICM_COMPRESS :
		return handler->VFW_compress((ICCOMPRESS*)lParam1, (DWORD)lParam2);

	// This is used to ask "Can we compress this?"
	case ICM_COMPRESS_QUERY :
		return handler->VFW_compress_query((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

	// What is our output format like?
	case ICM_COMPRESS_GET_FORMAT :
		return handler->VFW_compress_get_format((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

	// What's the maximum size of the output of this frame?
	case ICM_COMPRESS_GET_SIZE :
		return handler->VFW_compress_get_size((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

	// This seems to give us all kinds of timing info.
	// Number of frames, framerate, scaling factor to scale framerate by,
	// keyframe rate, etc. According to MS, the code should setup its
	// config when this message is recieved in preparation to compress.
	case ICM_COMPRESS_FRAMES_INFO :
		return handler->VFW_compress_frames_info((ICCOMPRESSFRAMES*)lParam1);

	// This is used so we can do any pre-compression setup. Constructors, init, etc.
	case ICM_COMPRESS_BEGIN :
		return handler->VFW_compress_begin((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

	// This is used for cleanup after we're done compressing frames. Destructors etc.
	// Note the params are defined as always null in vfw.h
	case ICM_COMPRESS_END :
		return handler->VFW_compress_end((int)lParam1, (int)lParam2);

// Decompression section.
	
	// Decompress this frame.
	case ICM_DECOMPRESS :
		return handler->VFW_decompress((ICDECOMPRESS*)lParam1, (DWORD)lParam2);

	// Can we decompress this?
	case ICM_DECOMPRESS_QUERY :
		return handler->VFW_decompress_query((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);
	// What format will this decompress to?
	case ICM_DECOMPRESS_GET_FORMAT :
		return handler->VFW_decompress_get_format((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);
		
	// Do any setup needed for decompression.
	case ICM_DECOMPRESS_BEGIN :
		return handler->VFW_decompress_begin((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);
		
	// Cleanup after decompression.
	// Note the params are always null in vfw.h
	case ICM_DECOMPRESS_END :
		return handler->VFW_decompress_end((int)lParam1, (int)lParam2);
		
	// Seems to be more stuff that's unsupported.
	case ICM_DECOMPRESS_GET_PALETTE :
	case ICM_DECOMPRESS_SET_PALETTE :
	case ICM_DECOMPRESSEX_QUERY:
	case ICM_DECOMPRESSEX_BEGIN:
	case ICM_DECOMPRESSEX_END:
	case ICM_DECOMPRESSEX:
		return ICERR_UNSUPPORTED;

	default:
		return DefDriverProc(dwDriverId, hDriver, uMsg, lParam1, lParam2);
	}
}
