
#include "DbgOut.h"

#ifdef _DEBUG
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

static DWORD g_dwTimeOffset = timeGetTime();

void WINAPI DbgStringOut(const TCHAR *pFormat,...)
{    	
  TCHAR szInfo[2000];

  // Format the variable length parameter list
  va_list va;
  va_start(va, pFormat);

  lstrcpy(szInfo, TEXT("CorePNG - "));
  wsprintf(szInfo + lstrlen(szInfo),
            TEXT("(tid %x) %8d : "),
            GetCurrentThreadId(), timeGetTime() - g_dwTimeOffset);

  _vstprintf(szInfo + lstrlen(szInfo), pFormat, va);
  lstrcat(szInfo, TEXT("\r\n"));
  OutputDebugString(szInfo);

  va_end(va);
};

#ifdef UNICODE
void WINAPI DbgLogInfo(const CHAR *pFormat,...)
{
    TCHAR szInfo[2000];

    // Format the variable length parameter list
    va_list va;
    va_start(va, pFormat);

    lstrcpy(szInfo, TEXT("TitleBar - "));
    wsprintf(szInfo + lstrlen(szInfo),
             TEXT("(tid %x) %8d : "),
             GetCurrentThreadId(), timeGetTime() - g_dwTimeOffset);

    CHAR szInfoA[2000];
    WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szInfoA, sizeof(szInfoA)/sizeof(CHAR), 0, 0);

    wvsprintfA(szInfoA + lstrlenA(szInfoA), pFormat, va);
    lstrcatA(szInfoA, "\r\n");

    WCHAR wszOutString[2000];
    MultiByteToWideChar(CP_ACP, 0, szInfoA, -1, wszOutString, sizeof(wszOutString)/sizeof(WCHAR));
    DbgOutString(wszOutString);

    va_end(va);
}
#endif
#endif

