// png2avi2png.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// Cpng2avi2pngApp:
// See png2avi2png.cpp for the implementation of this class
//

class Cpng2avi2pngApp : public CWinApp
{
public:
	Cpng2avi2pngApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cpng2avi2pngApp theApp;