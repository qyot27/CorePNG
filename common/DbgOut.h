
#ifndef _DBG_OUT_H_
#define _DBG_OUT_H_

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#ifdef _DEBUG
void WINAPI DbgStringOut(const TCHAR *pFormat,...);
#ifdef UNICODE
void WINAPI DbgLogInfo(const CHAR *pFormat,...)
#endif
#define NOTE(x) DbgStringOut(x)
#define NOTE1(x, y1) DbgStringOut(x, y1)
#define NOTE2(x, y1, y2) DbgStringOut(x, y1, y2)
#define NOTE3(x, y1, y2, y3) DbgStringOut(x, y1, y2, y3)
#define NOTE4(x, y1, y2, y3, y4) DbgStringOut(x, y1, y2, y3, y4)
#define NOTE5(x, y1, y2, y3, y4, y5) DbgStringOut(x, y1, y2, y3, y4, y5)
#define NOTE6(x, y1, y2, y3, y4, y5, y6) DbgStringOut(x, y1, y2, y3, y4, y5, y6)
#define NOTE10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10) DbgStringOut(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#define ODS(x) DbgStringOut(x)
#define ODS1(x, y1) DbgStringOut(x, y1)
#define ODS2(x, y1, y2) DbgStringOut(x, y1, y2)
#define ODS3(x, y1, y2, y3) DbgStringOut(x, y1, y2, y3)
#define ODS4(x, y1, y2, y3, y4) DbgStringOut(x, y1, y2, y3, y4)
#define ODS5(x, y1, y2, y3, y4, y5) DbgStringOut(x, y1, y2, y3, y4, y5)
#define ODS6(x, y1, y2, y3, y4, y5, y6) DbgStringOut(x, y1, y2, y3, y4, y5, y6)
#define ODS10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10) DbgStringOut(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#else
#define NOTE(x)
#define NOTE1(x, y1)
#define NOTE2(x, y1, y2)
#define NOTE3(x, y1, y2, y3)
#define NOTE4(x, y1, y2, y3)
#define NOTE5(x, y1, y2, y3, y4, y5)
#define NOTE6(x, y1, y2, y3, y4, y5, y6)
#define NOTE10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#define ODS(x)
#define ODS1(x, y1)
#define ODS2(x, y1, y2)
#define ODS3(x, y1, y2, y3)
#define ODS4(x, y1, y2, y3, y4)
#define ODS5(x, y1, y2, y3, y4, y5)
#define ODS6(x, y1, y2, y3, y4, y5, y6)
#define ODS10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#endif


#endif // _DBG_OUT_H_
