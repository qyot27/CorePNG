/*
 *  Part of The TCMP Matroska CDL, and Matroska Shell Extension
 *
 *  mdump.h
 *
 *  Copyright (C) Jory Stone - 2003
 *
 *  Based from sample code from http://www.eptacom.net/pubblicazioni/pub_eng/except.html
 *  Parts from VirtualDub's crash dialog
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * copying.txt included in the packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
*/

/*!
    \file mdump.h
		\version \$Id$
    \brief Crash Catcher
		\author Jory Stone     <jcsston @ toughguy.net>

*/

#include "mdump.h"

#ifndef TRACE_CRASH
#define TRACE_CRASH(x) 
//MessageBox(NULL, _T(x), _T("Debug Message"), MB_SYSTEMMODAL)
//debugLog = fopen("C:\\debug.txt", "wa"); fwrite(_T(x), 1, strlen(x), debugLog); fclose(debugLog);
// MessageBox(NULL, _T(x), _T("Debug Message"), MB_SYSTEMMODAL)
#endif

// Handle to this DLL itself.
extern HINSTANCE g_hInst;	

CrashCatcher::CrashCatcher(const TCHAR *szAppName )
{
	// if this assert fires then you have two instances of MiniDumper
	// which is not allowed
	assert(m_szAppName == NULL);

	m_szAppName = szAppName;

	m_prevFilter = ::SetUnhandledExceptionFilter( TopLevelFilter );
}

CrashCatcher::~CrashCatcher()
{
	::SetUnhandledExceptionFilter(m_prevFilter);
}

LONG CrashCatcher::TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{	
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = GetTopWindow(NULL);	

	std::basic_string<TCHAR> strResult;

#ifdef _DEBUG
	//while (true);
#endif

	std::basic_string<TCHAR> strDebugMsg;
	strDebugMsg = ProcessExceptionRecord(pExceptionInfo->ExceptionRecord);
	strDebugMsg += DumpRegisters(pExceptionInfo->ContextRecord);
	strDebugMsg += StackDump(pExceptionInfo->ContextRecord->Ebp, pExceptionInfo->ContextRecord->Eip);

	// Get System Info
	strDebugMsg += GetSystemInfomation();
	CrashInfo myCrash;
	myCrash.title = _T("Crash in ");
	myCrash.title += m_szAppName;
	myCrash.report = strDebugMsg;
	myCrash.filename = _T("corepng_vfw_crash_dump.zip");
	
	// ask the user if they want to save a dump file			
	int dlgChoice = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_CRASH), hParent, CrashDlgProc, (LPARAM)&myCrash);
	if (dlgChoice == IDIGNORE)
		return EXCEPTION_CONTINUE_EXECUTION;

	if (dlgChoice == IDOK) {	
		TRACE_CRASH("Creating zip archive\n");
		ZipArchiveWriter zipArchive(myCrash.filename.c_str());
		if (!zipArchive.IsOk())
			return EXCEPTION_EXECUTE_HANDLER;
		
		TRACE_CRASH("Searching for dbghelp.dll\n");
		// Try saving a dump file
		// firstly see if dbghelp.dll is around and has the function we need
		// look next to the EXE first, as the one in System32 might be old 
		// (e.g. Windows 2000)
		HMODULE hDll = NULL;
		TCHAR szDbgHelpPath[_MAX_PATH];

		if (GetModuleFileName(NULL, szDbgHelpPath, _MAX_PATH)) {
			TCHAR *pSlash = _tcsrchr(szDbgHelpPath, _T('\\'));
			if (pSlash) {
				_tcscpy(pSlash+1, _T("DBGHELP.DLL") );
				hDll = ::LoadLibrary( szDbgHelpPath );
			}
		}

		if (hDll == NULL) {
			TRACE_CRASH("loading any dbghelp.dll version we can \n");
			// load any version we can
			hDll = ::LoadLibrary( _T("DBGHELP.DLL"));
		}
		if (hDll) {
			TRACE_CRASH("found a dbghelp.dll\n");

			MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump" );
			// Got the dumper function?
			if (pDump) {		
				TRACE_CRASH("found the dumper function in dbghelp.dll\n");

				std::basic_string<TCHAR> strDumpPath;

				// work out a good place for the dump file
				if (!GetTempPath(255, szScratchPad))
					strDumpPath = _T("c:\\temp\\");
				strDumpPath = szScratchPad;

				// Add the app name to the dump filename
				strDumpPath += m_szAppName;

				strDumpPath += szScratchPad;

				strDumpPath += _T(".dmp");

				// create the file
				HANDLE hFile = ::CreateFile(strDumpPath.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

				if (hFile != INVALID_HANDLE_VALUE) {
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
					::CloseHandle(hFile);
					if (bOK) {						
						zipArchive.AddFile(strDumpPath.c_str());
					} else {
						_sntprintf(szScratchPad, 255, _T("Failed to save dump file to '%s' (error %d)"), strDumpPath.c_str(), GetLastError());
						myCrash.report += szScratchPad;
					}					
					::DeleteFile(strDumpPath.c_str());
				} else {
					_sntprintf(szScratchPad, 255, _T("Failed to create dump file '%s' (error %d)"), strDumpPath.c_str(), GetLastError());
					myCrash.report += szScratchPad;
				}
			} else {
				myCrash.report += _T("DBGHELP.DLL too old");
			}
		} else {
			myCrash.report += _T("DBGHELP.DLL not found");
		}

		TRACE_CRASH("Adding files to the zip archive\n");
		zipArchive.AddFile("report.txt", (unsigned char *)myCrash.report.c_str(), myCrash.report.length() * sizeof(TCHAR));
		zipArchive.AddFile("user_report.txt", (unsigned char *)myCrash.user_text.c_str(), myCrash.user_text.length() * sizeof(TCHAR));		

		TRACE_CRASH("Closing the zip archive\n");
		zipArchive.Close();

		_sntprintf(szScratchPad, 255, _T("Saved dump file to '%s'.\nPlease send this file to me 'jcsston' at vbman@toughguy.net."), myCrash.filename.c_str());
		strResult = szScratchPad;
		retval = EXCEPTION_EXECUTE_HANDLER;
	}


	if (!strResult.empty())
		::MessageBox(hParent, strResult.c_str(), m_szAppName, MB_OK);

	return retval;
}

std::basic_string<TCHAR> CrashCatcher::ProcessExceptionRecord(EXCEPTION_RECORD *pExceptionRecord) {
	std::basic_string<TCHAR> strDebugMsg;
	
	while (pExceptionRecord != NULL) {
		strDebugMsg = _T("Exception Type: ");
		if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
			// EXCEPTION_ACCESS_VIOLATION
			strDebugMsg += _T("Access Violation");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;
			if (pExceptionRecord->NumberParameters >= 2) {
				if (pExceptionRecord->ExceptionInformation[0] == 0) {
					strDebugMsg += _T("Tried to read address ");
				} else if (pExceptionRecord->ExceptionInformation[0] == 1) {
					strDebugMsg += _T("Tried to write address ");
				}
				_sntprintf(szScratchPad, 255, _T("%p\r\n"), (void *)pExceptionRecord->ExceptionInformation[1]);
				strDebugMsg += szScratchPad;
			}				 

		} else if (pExceptionRecord->ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO) {
			// EXCEPTION_FLT_DIVIDE_BY_ZERO
			strDebugMsg += _T("Float Divide by Zero");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;

		} else if (pExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) {
			// EXCEPTION_INT_DIVIDE_BY_ZERO
			strDebugMsg += _T("Integer Divide by Zero");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;
		}	else if (pExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
			// EXCEPTION_ILLEGAL_INSTRUCTION
			strDebugMsg += _T("Illegal Instruction (running MMX code on non-MMX cpu?)");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;

		}	else if (pExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION) {
			// EXCEPTION_PRIV_INSTRUCTION
			strDebugMsg += _T("Illegal Instruction (Instruction not allowed in the current machine mode?)");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;

		}	else if (pExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
			// EXCEPTION_STACK_OVERFLOW
			strDebugMsg += _T("Stack Overflow");
			_sntprintf(szScratchPad, 255, _T(" at Address %p\r\n"), pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;

		}	else if (pExceptionRecord->ExceptionCode == 0xe06d7363) {
			// std::exception
			strDebugMsg += _T("std::exception ");
			std::exception *ex = (std::exception *)pExceptionRecord->ExceptionInformation[1];
			if (ex != NULL) {
				const char *ex_what = ex->what();
				if (ex_what != NULL) {	
					_sntprintf(szScratchPad, 255, _T("'%S' at Address %p\r\n"), ex_what, pExceptionRecord->ExceptionAddress);
					strDebugMsg += szScratchPad;
				}
			}						

		} else {
			strDebugMsg += _T("Unknown Exception");
			_sntprintf(szScratchPad, 255, _T("(%i) at Address %p\r\n"), pExceptionRecord->ExceptionCode, pExceptionRecord->ExceptionAddress);
			strDebugMsg += szScratchPad;
		}
		
		_sntprintf(szScratchPad, 255, _T("Exception Flags: %i \r\n"), pExceptionRecord->ExceptionFlags);
		strDebugMsg += szScratchPad;
		
		strDebugMsg += _T("More Exception Infomation: \r\n");
		for (int i = 0; i < 15; i++) {
			_sntprintf(szScratchPad, 255, _T("[%02i]: %08X \r\n"), i, pExceptionRecord->ExceptionInformation[i]);
			strDebugMsg += szScratchPad;
		}
		pExceptionRecord = pExceptionRecord->ExceptionRecord;
	}			

	return strDebugMsg;
};

std::basic_string<TCHAR> CrashCatcher::DumpRegisters(CONTEXT *pContext) 
{
	std::basic_string<TCHAR> s = _T("\r\nRegister Dump\r\n");

	_sntprintf(szScratchPad, 255, _T("EAX = %08lx\r\n"), pContext->Eax);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("EBX = %08lx\r\n"), pContext->Ebx);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("ECX = %08lx\r\n"), pContext->Ecx);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("EDX = %08lx\r\n"), pContext->Edx);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("EBP = %08lx\r\n"), pContext->Ebp);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("DS:ESI = %04x:%08lx\r\n"), pContext->SegDs, pContext->Esi);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("ES:EDI = %04x:%08lx\r\n"), pContext->SegEs, pContext->Edi);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("SS:ESP = %04x:%08lx\r\n"), pContext->SegSs, pContext->Esp);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("CS:EIP = %04x:%08lx\r\n"), pContext->SegCs, pContext->Eip);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("FS = %04x\r\n"), pContext->SegFs);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("GS = %04x\r\n"), pContext->SegGs);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("EFLAGS = %08lx\r\n"), pContext->EFlags);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("FPUCW = %04x\r\n"), pContext->FloatSave.ControlWord);
	s += szScratchPad;
	_sntprintf(szScratchPad, 255, _T("FPUTW = %04x\r\n"), pContext->FloatSave.TagWord);
	s += szScratchPad;

	// extract out MMX registers

	int tos = (pContext->FloatSave.StatusWord & 0x3800)>>11;

	for(int i=0; i<8; i++) {
		long *pReg = (long *)(pContext->FloatSave.RegisterArea + 10*((i-tos) & 7));

		_sntprintf(szScratchPad, 255, _T("MM%c = %08lx%08lx\r\n"), i+'0', pReg[1], pReg[0]);
		s += szScratchPad;
	}

	return s;
};

std::basic_string<TCHAR> CrashCatcher::StackDump(DWORD EBP, DWORD EIP) 
{
#ifdef USING_MAPPING_FILE
	std::vector<MappingFileEntry> mapFile;
	LoadMapping(mapFile);
#endif

	std::basic_string<TCHAR> s = _T("\r\nStack Trace \r\n");

	BOOL EBP_OK = TRUE;
	while (EBP_OK) {
		// Check if EBP is a good address
		// I'm expecting a standard stack frame, so
		// EPB must be aligned on a DWORD address
		// and it should be possible to read 8 bytes
		// starting at it (next EBP, caller).
		if ((DWORD)EBP & 3)
			EBP_OK = FALSE;

		if (EBP_OK && IsBadReadPtr((void*)EBP, 8))
			EBP_OK = FALSE;

		if (EBP_OK) {
			BYTE* caller = EIP ? (BYTE*)EIP : *((BYTE**)EBP + 1) ;
			EBP = EIP ? EBP : *(DWORD*)EBP ;
			if( EIP )
				EIP = 0 ; // So it is considered just once

			// Get the instance handle of the module where caller belongs to
			MEMORY_BASIC_INFORMATION mbi ;
			VirtualQuery(caller, &mbi, sizeof(mbi)) ;

			// The instance handle is equal to the allocation base in Win32
			HINSTANCE hInstance = (HINSTANCE)mbi.AllocationBase ;

			// If EBP is valid, hInstance is not 0
			if(hInstance == 0) {
				EBP_OK = FALSE ;
			} else {
				s += GetAddressInfo(caller, hInstance);				  
			}
		}
	}
	return s;
};

std::basic_string<TCHAR> CrashCatcher::GetAddressInfo(const BYTE* caller, HINSTANCE hInstance)
{
	std::basic_string<TCHAR> s;

	static TCHAR moduleFilename[256];
  GetModuleFileName(hInstance, moduleFilename, 255);
	
	DWORD relativeAddress = caller - (BYTE*)hInstance;
	
	_sntprintf(szScratchPad, 255, _T("%08X : %s \r\n"), (DWORD)caller, moduleFilename);
	s = szScratchPad;

#ifdef USING_MAPPING_FILE
	s += LookupMapping(sourceMap, (void *)relativeAddress);
	s += _T(" - ");
	s += moduleFilename;
	s += _T("\r\n");
#endif
	return s;
};

#ifdef USING_MAPPING_FILE
void CrashCatcher::LoadMapping(std::vector<MappingFileEntry> &targetMap) 
{	
	FILE *mappingFile = fopen("C:\\Documents and Settings\\Jory\\My Documents\\MatroskaProp.map.mapping", "rb");
	
	MappingFileEntry functionEntry;	
	WORD functionStrLength;
	if (mappingFile != NULL) {
		while (fread(&functionEntry.functionAddress, 1, sizeof(DWORD), mappingFile) == sizeof(DWORD)) {
			fread(&functionStrLength, 1, sizeof(WORD), mappingFile);
			
			functionEntry.functionName.resize(functionStrLength);
			fread((void *)functionEntry.functionName.c_str(), 1, functionStrLength, mappingFile);
			targetMap.push_back(functionEntry);
		}
	}
};

std::basic_string<TCHAR> CrashCatcher::LookupMapping(std::vector<MappingFileEntry> &sourceMap, void *address) 
{	
	static std::basic_string<TCHAR> s;
	static size_t lastPos = 0;
	
	for (;lastPos < sourceMap.size(); lastPos++) {
		MappingFileEntry &fileEntry = sourceMap.at(lastPos);
		if ((BYTE *)fileEntry.functionAddress > (BYTE *)address+0x1c001000) {
			s.resize(fileEntry.functionName.length());
			for (WORD c = 0; c < fileEntry.functionName.length(); c++)
				s[c] = fileEntry.functionName[c];
			break;
		}
	}
	
	return s;
};
#endif

std::basic_string<TCHAR> CrashCatcher::GetSystemInfomation()
{
	std::basic_string<TCHAR> s;
	
	s = _T("\r\nSystem Infomation \r\n");
	s += _T("Running under ");
	OSVERSIONINFO osVer;
	osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osVer)) {
		if (osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 0) {
			s += _T("Windows 95 ");

		} else if (osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 10) {
			s += _T("Windows 98 ");

		} else if (osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 90) {
			s += _T("Windows ME ");

		} else if (osVer.dwMajorVersion == 3 && osVer.dwMinorVersion == 51) {
			s += _T("Windows NT 3.51 ");

		} else if (osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 0) {
			s += _T("Windows NT 4.0 ");

		} else if (osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 0) {
			s += _T("Windows NT 2000 ");

		} else if (osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 1) {
			s += _T("Windows XP ");
		}
		_sntprintf(szScratchPad, 255, _T("\r\nBuild %i "), osVer.dwBuildNumber);
		s += szScratchPad;
		s += osVer.szCSDVersion;

		s += _T("\r\n  Comctl32.dll      version ");
		s += GetDllVersion(_T("Comctl32.dll"));
		s += _T("\r\n  Shell32.dll       version ");
		s += GetDllVersion(_T("Shell32.dll"));
		s += _T("\r\n  Shlwapi.dll       version ");
		s += GetDllVersion(_T("Shlwapi.dll"));		
		s += _T("\r\n  DbgHelp.dll       version ");
		s += GetDllVersion(_T("DbgHelp.dll"));	
		s += _T("\r\n  MatroskaProp.dll  version ");
		s += GetDllVersion(_T("MatroskaProp.dll"));				
	}

	s += _T("\r\n");
	return s;
};

LPCTSTR CrashCatcher::GetDllVersion(LPCTSTR lpszDllName)
{
	static TCHAR versionStr[65];
	versionStr[0] = 0;
	
	HINSTANCE hinstDll = LoadLibrary(lpszDllName);

	if(hinstDll) {
		DLLGETVERSIONPROC pDllGetVersion;

		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

		/*Because some DLLs might not implement this function, you
		must test for it explicitly. Depending on the particular 
		DLL, the lack of a DllGetVersion function can be a useful
		indicator of the version.
		*/
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;

			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);

			hr = (*pDllGetVersion)(&dvi);

			if(SUCCEEDED(hr))
			{
				_sntprintf(versionStr, 64, _T("%i, %i, %i, %i"), dvi.dwMajorVersion, dvi.dwMinorVersion, dvi.dwBuildNumber, dvi.dwPlatformID);
			}
		}

		FreeLibrary(hinstDll);
	}
	return versionStr;
}

BOOL CALLBACK CrashCatcher::CrashDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CrashInfo *crashData = (CrashInfo *)GetWindowLong(hwndDlg, DWL_USER);	
  switch (uMsg)
  {
		case WM_INITDIALOG:
		{
			crashData = (CrashInfo *)lParam;
			// Store tha crash data
			SetWindowLong(hwndDlg, DWL_USER, (LONG)crashData);

			// Display the text
			SetWindowText(hwndDlg, crashData->title.c_str());
			SetDlgItemText(hwndDlg, IDC_STATIC_CRASH_TITLE, _T("Crash incoming!!!\nWould you like to save a crash report dump file? ")
		    _T("This will help in finding out what went wrong and fixing it.\n")
		    _T("Note that the dump file will have some infomation about the ")
		    _T("crash, anything you see displayed by the Shell Ext could be in the dump file.")
      );
      
			SetDlgItemText(hwndDlg, IDC_EDIT_CRASH_REPORT, crashData->report.c_str());
			break;
		}
		case WM_COMMAND:
			// Process button
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_SAVE_DUMP:
				{
					switch (HIWORD(wParam)) 
					{ 
						case BN_CLICKED: 
						{
							OPENFILENAME saveFileAs;
							TCHAR fileName[1025];
							memset(&saveFileAs, 0, sizeof(OPENFILENAME));
							memset(fileName, 0, 1024);
							_tcscpy(fileName, crashData->filename.c_str());
							saveFileAs.lStructSize =  sizeof(OPENFILENAME);
							saveFileAs.hwndOwner = hwndDlg;									
							saveFileAs.lpstrFile = fileName;
							saveFileAs.nMaxFile	= 1024;
							saveFileAs.lpstrFilter = _T("Zip Files(*.zip)\0 *.zip\0\0");
							saveFileAs.lpstrTitle	= _T("Save Crash Report To...");
							saveFileAs.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
							if (GetSaveFileName(&saveFileAs)) {
								crashData->filename = fileName;
								EndDialog(hwndDlg, IDOK);
							}					
							break;
						}
					}
					break;
				}
				case IDIGNORE:
					switch (HIWORD(wParam)) 
					{ 
						case BN_CLICKED: 
						{
							EndDialog(hwndDlg, IDIGNORE);
							break;
						}
					}
					break;			
				case IDCANCEL:
					switch (HIWORD(wParam)) 
					{ 
						case BN_CLICKED: 
						{
							EndDialog(hwndDlg, IDCANCEL);
							break;
						}
					}
					break;	
			}
			break;
		default:
			break;
			//Nothing for now
	}
  return FALSE;
}

TCHAR CrashCatcher::szScratchPad[] = {NULL};
const TCHAR *CrashCatcher::m_szAppName = NULL;
