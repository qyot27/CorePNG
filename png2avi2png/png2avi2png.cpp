// png2avi2png.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "png2avi2png.h"
#include "png2avi2pngDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cpng2avi2pngApp

BEGIN_MESSAGE_MAP(Cpng2avi2pngApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cpng2avi2pngApp construction

Cpng2avi2pngApp::Cpng2avi2pngApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only Cpng2avi2pngApp object

Cpng2avi2pngApp theApp;


// Cpng2avi2pngApp initialization

BOOL Cpng2avi2pngApp::InitInstance()
{
	AVIFileInit();

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();	

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("CorePNG"));

	Cpng2avi2pngDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int Cpng2avi2pngApp::ExitInstance()
{
	AVIFileExit();
	return CWinApp::ExitInstance();
};

bool AfxDoIdle() 
{
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!AfxGetThread()->PumpMessage())
			return false;
	}
	LONG lIdle = 0;
	while (AfxGetApp()->OnIdle(lIdle++)) { 
		Sleep(10); 
	};

	return true;
};
