#pragma once


// Cpng2avi2pngOptionsDlg dialog

class Cpng2avi2pngOptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(Cpng2avi2pngOptionsDlg)

public:
	Cpng2avi2pngOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~Cpng2avi2pngOptionsDlg();
	void SetPngSaverProgram(CString newPngSaverProgram, CString newPngSaverProgramParam) {
		pngSaverProgram = newPngSaverProgram;		
		pngSaverProgramParam = newPngSaverProgramParam;
	}
// Dialog Data
	enum { IDD = IDD_DIALOG_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString pngSaverProgram;
	afx_msg void OnEnChangeEditOptionsPngSaver();
	CString pngSaverProgramParam;
	afx_msg void OnBnClickedButtonBrowsePngSaver();
};
