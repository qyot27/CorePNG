// png2avi2pngOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "png2avi2png.h"
#include "png2avi2pngOptionsDlg.h"
#include ".\png2avi2pngoptionsdlg.h"


// Cpng2avi2pngOptionsDlg dialog

IMPLEMENT_DYNAMIC(Cpng2avi2pngOptionsDlg, CDialog)
Cpng2avi2pngOptionsDlg::Cpng2avi2pngOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cpng2avi2pngOptionsDlg::IDD, pParent)
	, pngSaverProgram(_T(""))
	, pngSaverProgramParam(_T(""))
{
}

Cpng2avi2pngOptionsDlg::~Cpng2avi2pngOptionsDlg()
{
}

void Cpng2avi2pngOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OPTIONS_PNG_SAVER, pngSaverProgram);
	DDX_Text(pDX, IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, pngSaverProgramParam);
}


BEGIN_MESSAGE_MAP(Cpng2avi2pngOptionsDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_OPTIONS_PNG_SAVER, OnEnChangeEditOptionsPngSaver)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_PNG_SAVER, OnBnClickedButtonBrowsePngSaver)
END_MESSAGE_MAP()


// Cpng2avi2pngOptionsDlg message handlers

void Cpng2avi2pngOptionsDlg::OnEnChangeEditOptionsPngSaver()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void Cpng2avi2pngOptionsDlg::OnBnClickedButtonBrowsePngSaver()
{
	CFileDialog input(TRUE, 0, pngSaverProgram, OFN_HIDEREADONLY, _T("Programs (*.exe)|*.exe|All Files (*.*)|*.*||\0"));
	if (input.DoModal() == IDOK) {
		pngSaverProgram = input.GetPathName();
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, pngSaverProgram);
	}
}
