// png2avi2pngDlg.cpp : implementation file
//

#include "stdafx.h"
#include "png2avi2png.h"
#include "png2avi2pngDlg.h"
#include ".\png2avi2pngdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cpng2avi2pngDlg dialog



Cpng2avi2pngDlg::Cpng2avi2pngDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cpng2avi2pngDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cpng2avi2pngDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cpng2avi2pngDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_EDIT_INPUT_FILE, OnEnChangeEditInputFile)
	ON_BN_CLICKED(IDC_BUTTON_INPUT_BROWSE, OnBnClickedButtonInputBrowse)
	ON_BN_CLICKED(IDC_BUTTON_OUTPUT_BROWSE, OnBnClickedButtonOutputBrowse)
	ON_EN_CHANGE(IDC_EDIT_OUTPUT_FILE, OnEnChangeEditOutputFile)
	ON_BN_CLICKED(IDC_BUTTON_START, OnBnClickedButtonStart)
END_MESSAGE_MAP()


// Cpng2avi2pngDlg message handlers

BOOL Cpng2avi2pngDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cpng2avi2pngDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cpng2avi2pngDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cpng2avi2pngDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cpng2avi2pngDlg::OnEnChangeEditInputFile()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	GetDlgItemText(IDC_EDIT_INPUT_FILE, m_InputFilename);
}

void Cpng2avi2pngDlg::OnBnClickedButtonInputBrowse()
{
	// TODO: Add your control notification handler code here
	CFileDialog input(TRUE, _T("AVI Files (*.avi)\0*.avi\0All Files (*.*)\0*.*\0\0"), 0, OFN_FILEMUSTEXIST);
	if (input.DoModal() == IDOK) {
		m_InputFilename = input.GetPathName();
		SetDlgItemText(IDC_EDIT_INPUT_FILE, m_InputFilename);
	}
}

void Cpng2avi2pngDlg::OnEnChangeEditOutputFile()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	GetDlgItemText(IDC_EDIT_OUTPUT_FILE, m_OutputFilename);
}

void Cpng2avi2pngDlg::OnBnClickedButtonOutputBrowse()
{
	// TODO: Add your control notification handler code here
	CFileDialog input(FALSE, _T("PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0\0"));
	if (input.DoModal() == IDOK) {
		m_OutputFilename = input.GetPathName();
		SetDlgItemText(IDC_EDIT_OUTPUT_FILE, m_OutputFilename);
	}
}


void Cpng2avi2pngDlg::OnBnClickedButtonStart()
{
	// TODO: Add your control notification handler code here
	CWaitCursor wait;
	SendDlgItemMessage(IDC_PROGRESS_GAUGE, PBM_SETPOS, 0, 0);
	HRESULT ret;
	AVIFileInit();
	ret = AVIFileOpen(&m_AVIFile, m_InputFilename, OF_SHARE_DENY_WRITE, 0L);
	if (ret != AVIERR_OK) {
		//(_T("AVIFileOpen failed, unable to open %s :"), m_InputFilename);
		return;
	}

	BITMAPINFO *bmh = NULL;
	LONG currentAVIStream = 0;
	bool bMoreStreams = true;
	while (bMoreStreams) {		
		// Go through the streams
		ret = AVIFileGetStream(m_AVIFile, &m_AVIStream, 0, currentAVIStream);
		if (ret != AVIERR_OK) {
			if (currentAVIStream == 0) {
				//wxLogError(_T("AVIFileGetStream failed, unable to get any stream!"));
				return;
			}
			break;
		}		
		ret = AVIStreamInfo(m_AVIStream, &m_AVIStreamInfo, sizeof(m_AVIStreamInfo));
		if (ret != AVIERR_OK) {
			//wxLogError(_T("AVIStreamInfo failed, unable to get stream info!"));
			return;
		}

		if (m_AVIStreamInfo.fccType == streamtypeVIDEO) {						
			LONG lSize;
			ret = AVIStreamReadFormat(m_AVIStream, AVIStreamStart(m_AVIStream), NULL, &lSize);
			if (ret != AVIERR_OK) {
				return;
			}
			if (bmh != NULL)
				free(bmh);

			bmh = (BITMAPINFO *)malloc(lSize);
			ZeroMemory(bmh, lSize);

			ret = AVIStreamReadFormat(m_AVIStream, 0, bmh, &lSize);
			if (ret != AVIERR_OK) {
				//wxLogError(_T("AVIStreamReadFormat failed!"));
				return;
			}
			break;
		}
		currentAVIStream++;
	}
	if (bmh != NULL) {
		// Found a video stream
		if (bmh->bmiHeader.biCompression == MAKE_FOURCC("PNG1")) {
			// w00t it's a PNG avi !!!
			BYTE *frameBuffer = NULL;
			LONG frameBufferSize = 0;
			LONG frameBufferTotalSize = 0;
			LONG frameSample = 0;
			LONG frameLength = AVIStreamLength(m_AVIStream);
			LONG samplesRead = 0;			
			while (ret == AVIERR_OK) {
				ret = AVIStreamRead(m_AVIStream, frameSample, AVISTREAMREAD_CONVENIENT, frameBuffer, frameBufferTotalSize, &frameBufferSize, &samplesRead);
				if (frameBufferTotalSize < frameBufferSize) {
					//wxLogDebug(_T("\nFrame Buffer was too small for AVIStreamRead.\n"));
					frameBufferTotalSize = frameBufferSize+1;
					delete frameBuffer;
					frameBuffer = new BYTE[frameBufferTotalSize+1];					
					ret = AVIStreamRead(m_AVIStream, frameSample, AVISTREAMREAD_CONVENIENT, frameBuffer, frameBufferTotalSize, &frameBufferSize, &samplesRead);
				}				
				if (ret != AVIERR_OK) {
					//wxLogError(_T("AVIStreamRead returned error."));
					return;
				}				

				if (frameBufferSize > 0) {
					CString outputName;		
					outputName.Format(_T("-%04i.png"), frameSample);
					outputName = m_OutputFilename + outputName;

					CFile output(outputName, CFile::modeCreate|CFile::modeWrite);
					output.Write(frameBuffer, frameBufferSize);					
				}
				SendDlgItemMessage(IDC_PROGRESS_GAUGE, PBM_SETPOS, (100 / (float)frameLength) * (float)frameSample, 0);
				{
					MSG msg;
					while (::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE)) 
						if (!AfxGetThread()->PumpMessage()) 
							return;

					// let MFC do its idle processing
					LONG lIdle = 0;
					while (AfxGetApp()->OnIdle(lIdle++));  
				}
				frameSample++;
			}
		}
	}
	/*CAviFile input;
	input.Open(m_InputFilename);
	BITMAPINFO *bmp = input.GetVideoFormat(0);
	if (bmp == NULL)
		return;
	if (bmp->bmiHeader.biCompression == MAKE_FOURCC("PNG1")) {
		long currentFrame = 0;
		long currentFrameSize = 0;
		BITMAPINFOHEADER *bmh = NULL;
		BYTE *buffer = new BYTE[bmp->bmiHeader.biSizeImage];

		input.StartVideoRetrieve(0);
		input.GetVideoFrame(0, currentFrame, &bmh);

		CString outputName;		
		outputName.Format(_T("-%04i"), currentFrame);
		outputName = m_OutputFilename + outputName;

		CFile output(outputName, CFile::modeCreate|CFile::modeWrite);
		output.Write(buffer, currentFrameSize);

		delete [] buffer;
	}*/
	/*m_AviFile = AVI_open_input_file(m_InputFilename, 1);
	if (m_AviFile != NULL) {		
		int keyframeFlag = 0;
		long currentFrame = 0;
		long currentFrameSize = 0;
		BYTE *buffer = new BYTE[m_AviFile->bitmap_info_header->bi_size_image];

		//AVI_set_video_position(m_AviFile, currentFrame);
		currentFrameSize = AVI_frame_size(m_AviFile, currentFrame);
		currentFrame = AVI_read_frame(m_AviFile, (char *)buffer, &keyframeFlag);		

		CString outputName;		
		outputName.Format(_T("-%04i"), currentFrame);
		outputName = m_OutputFilename + outputName;

		CFile output(outputName, CFile::modeCreate|CFile::modeWrite);
		output.Write(buffer, currentFrameSize);

		delete [] buffer;
	}*/
}
