// png2avi2pngDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#define SWAP_LONG(Var)		Var = *(long*)		SwapBytes((void*)&Var, sizeof(long))
void *SwapBytes(void* Addr, const int Nb);

class CAVI2PNGProcessing;

// Cpng2avi2pngDlg dialog
class Cpng2avi2pngDlg : public CDialog
{
// Construction
public:
	Cpng2avi2pngDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PNG2AVI2PNG_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	PAVIFILE m_AVIFile;
	PAVISTREAM m_AVIStream;
	AVISTREAMINFO m_AVIStreamInfo;		

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOutputBrowse();
	afx_msg void OnEnChangeEditOutputFile();
	afx_msg void OnBnClickedButtonStart();
protected:
	CProgressCtrl m_ProgressCtrl;

protected:
	CButton m_ButtonStart;
	CButton m_RadioAVI2PNG;

	friend CAVI2PNGProcessing;
public:
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonRemove();
protected:
	CListBox m_InputList;
public:
	afx_msg void OnBnClickedButtonOptions();
protected:
	CString m_OutputFilename;
};

class CAVI2PNGProcessing {
protected:
	CWaitCursor m_wait;
	Cpng2avi2pngDlg *m_parent;
public:
	CAVI2PNGProcessing(Cpng2avi2pngDlg *parent) {
		m_parent = parent;
		m_wait.Restore();
	
		m_parent->m_ButtonStart.SetWindowText(_T("Stop"));	
		m_parent->m_ProgressCtrl.SetPos(0);
		m_parent->SetDlgItemText(IDC_STATIC_PROGRESS, _T("Extracting..."));
	};
	~CAVI2PNGProcessing() {
		m_parent->SetDlgItemText(IDC_STATIC_PROGRESS, _T("Done."));
		m_parent->m_ButtonStart.SetWindowText(_T("Start"));	
	};
};

class PNGParser {
public:
	PNGParser(CFile &input);
	
	long width;
	long height;
	long size;
	unsigned char bitdepth;
};
