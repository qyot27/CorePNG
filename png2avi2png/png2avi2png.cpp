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

#define REG_KEY "Software\\CorePNG\\png2avi2png\\"
DWORD CorePNG_GetRegistryValue(const char *value_key, DWORD default_value)
{	
	DWORD ret_value = default_value;
	HKEY key_handle = NULL;
	DWORD lpType = NULL;
	DWORD state = 0;

	RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_handle, &state);
	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, temp, 0, reg_key, 1024, NULL); 
	if(state == 2)
	{
		DWORD size = sizeof(ret_value);
		RegQueryValueEx(key_handle, value_key, 0, &lpType, (BYTE*)&ret_value, &size);
	}
	RegCloseKey(key_handle);
	return ret_value;
}

void CorePNG_SetRegistryValue(const char *value_key, DWORD the_value)
{
	HKEY key_handle = NULL;
	DWORD lpType = NULL;
	DWORD state = 0;
	SECURITY_ATTRIBUTES sa = {sizeof(sa), 0,1};

	RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, "", 0, KEY_WRITE, &sa, &key_handle, &state);

	DWORD size = sizeof(the_value);
	RegSetValueEx(key_handle, value_key, 0, REG_DWORD, (CONST BYTE*)&the_value, size);
	//char *err_key = new char[1024];
	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, temp, 0, err_key, 1024, NULL); 
	RegCloseKey(key_handle);
};

const char *CorePNG_GetRegistryValueStr(const char *value_key, const char *default_value)
{	
	HKEY key_handle = NULL;
	DWORD lpType = NULL;
	DWORD state = 0;
	static char key_text[1025];
	DWORD size = 1024;

	ZeroMemory(key_text, MAX_PATH);
	RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_handle, &state);
	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, temp, 0, reg_key, 1024, NULL); 		
	if(state == REG_OPENED_EXISTING_KEY) {
		RegQueryValueExA(key_handle, value_key, 0, &lpType, (BYTE*)key_text, &size);	
	} else if (state == REG_CREATED_NEW_KEY) {
		if (default_value != NULL)
			lstrcpyA(key_text, default_value);
		else
			lstrcpyA(key_text, "");
	}

	RegCloseKey(key_handle);
	return key_text;
}

void CorePNG_SetRegistryValueStr(const char *value_key, const char *the_value)
{
	HKEY key_handle = NULL;
	DWORD lpType = NULL;
	DWORD state = 0;
	SECURITY_ATTRIBUTES sa = {sizeof(sa), 0,1};

	RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0, KEY_WRITE, &sa, &key_handle, &state);

	DWORD size = lstrlenA(the_value)+1;
	RegSetValueExA(key_handle, value_key, 0, REG_SZ, (CONST BYTE*)the_value, size);
	
	//char err_key[1024];
	//FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, err_key, 1024, NULL); 

	RegCloseKey(key_handle);
};
