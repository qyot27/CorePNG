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

BOOL Cpng2avi2pngOptionsDlg::OnInitDialog()
{
	BOOL ret = CDialog::OnInitDialog();

	m_PNGSaverPresetComboBox.AddString(_T("PNGCrush"));
	m_PNGSaverPresetComboBox.AddString(_T("advpng"));
	m_PNGSaverPresetComboBox.AddString(_T("OptiPNG"));
	m_PNGSaverPresetComboBox.AddString(_T("PNGOUT"));
	m_PNGSaverPresetComboBox.AddString(_T("pngrewrite"));

	return ret;
}
void Cpng2avi2pngOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OPTIONS_PNG_SAVER, pngSaverProgram);
	DDX_Text(pDX, IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, pngSaverProgramParam);
	DDX_Control(pDX, IDC_COMBO_OPTIONS_PNG_SAVER_PRESETS, m_PNGSaverPresetComboBox);
}


BEGIN_MESSAGE_MAP(Cpng2avi2pngOptionsDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_OPTIONS_PNG_SAVER, OnEnChangeEditOptionsPngSaver)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_PNG_SAVER, OnBnClickedButtonBrowsePngSaver)
	ON_EN_CHANGE(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, OnEnChangeEditOptionsPngSaverParam)
	ON_CBN_SELCHANGE(IDC_COMBO_OPTIONS_PNG_SAVER_PRESETS, OnCbnSelchangeComboOptionsPngSaverPresets)
END_MESSAGE_MAP()


// Cpng2avi2pngOptionsDlg message handlers

void Cpng2avi2pngOptionsDlg::OnEnChangeEditOptionsPngSaver()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	GetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, pngSaverProgram);
}

void Cpng2avi2pngOptionsDlg::OnBnClickedButtonBrowsePngSaver()
{
	CFileDialog input(TRUE, 0, pngSaverProgram, OFN_HIDEREADONLY, _T("Programs (*.exe)|*.exe|All Files (*.*)|*.*||\0"));
	if (input.DoModal() == IDOK) {
		pngSaverProgram = input.GetPathName();
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, pngSaverProgram);
	}
}

void Cpng2avi2pngOptionsDlg::OnEnChangeEditOptionsPngSaverParam()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	GetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, pngSaverProgramParam);
}

void Cpng2avi2pngOptionsDlg::OnCbnSelchangeComboOptionsPngSaverPresets()
{
	// TODO: Add your control notification handler code here	
	CString selectedItem;
	m_PNGSaverPresetComboBox.GetLBText(m_PNGSaverPresetComboBox.GetCurSel(), selectedItem);
	if (selectedItem == _T("PNGCrush")) {
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, _T("pngcrush.exe"));
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, _T("%1 %2"));

	} else if (selectedItem == _T("advpng")) {
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, _T("advpng.exe"));
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, _T("-z -4 %1"));

	} else if (selectedItem == _T("OptiPNG")) {
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, _T("optipng.exe"));
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, _T("-o7 %1"));

	} else if (selectedItem == _T("PNGOUT")) {
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, _T("pngout.exe"));
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, _T("%1 %2"));

	} else if (selectedItem == _T("pngrewrite")) {
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER, _T("pngrewrite.exe"));
		SetDlgItemText(IDC_EDIT_OPTIONS_PNG_SAVER_PARAM, _T("%1 %2"));

	}
}
