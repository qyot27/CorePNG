// png2avi2pngDlg.h : header file
//

#pragma once


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
	CString m_InputFilename;
	CString m_OutputFilename;

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
	afx_msg void OnEnChangeEditInputFile();
	afx_msg void OnBnClickedButtonInputBrowse();
	afx_msg void OnBnClickedButtonOutputBrowse();
	afx_msg void OnEnChangeEditOutputFile();
	afx_msg void OnBnClickedButtonStart();
};
