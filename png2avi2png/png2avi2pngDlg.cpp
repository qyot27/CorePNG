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
	, m_OutputFilename(_T(""))
{
	
}

void Cpng2avi2pngDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_GAUGE, m_ProgressCtrl);
	DDX_Control(pDX, IDC_BUTTON_START, m_ButtonStart);
	DDX_Control(pDX, IDC_RADIO_AVI2PNG, m_RadioAVI2PNG);
	DDX_Control(pDX, IDC_LIST_INPUT, m_InputList);
	DDX_Text(pDX, IDC_EDIT_OUTPUT_FILE, m_OutputFilename);
}

BEGIN_MESSAGE_MAP(Cpng2avi2pngDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_OUTPUT_BROWSE, OnBnClickedButtonOutputBrowse)
	ON_EN_CHANGE(IDC_EDIT_OUTPUT_FILE, OnEnChangeEditOutputFile)
	ON_BN_CLICKED(IDC_BUTTON_START, OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_OPTIONS, OnBnClickedButtonOptions)
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
	SendDlgItemMessage(IDC_RADIO_AVI2PNG, BM_SETCHECK, BST_CHECKED, 0);
	
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
	CFileDialog input(FALSE);
	if (input.DoModal() == IDOK) {
		m_OutputFilename = input.GetPathName();
		SetDlgItemText(IDC_EDIT_OUTPUT_FILE, m_OutputFilename);
	}
}


void Cpng2avi2pngDlg::OnBnClickedButtonStart()
{
	// TODO: Add your control notification handler code here
	HRESULT ret;		
	CString buttonTxt;
	m_ButtonStart.GetWindowText(buttonTxt);
	if (buttonTxt == _T("Start")) {
		CAVI2PNGProcessing busy(this);
								
		if (IsDlgButtonChecked(IDC_RADIO_AVI2PNG) || IsDlgButtonChecked(IDC_RADIO_AVI2AVI)) {
			CString inputFilename;
			m_InputList.GetText(0, inputFilename);
			ret = AVIFileOpen(&m_AVIFile, inputFilename, OF_SHARE_DENY_WRITE|OF_READ, 0L);
			if (ret != AVIERR_OK) {
				CString err;
				err.Format(_T("AVIFileOpen failed, unable to open: '%s'"), inputFilename);
				MessageBox(err);
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
						MessageBox(_T("AVIFileGetStream failed, unable to get any stream!"));
						return;
					}
					break;
				}		
				ret = AVIStreamInfo(m_AVIStream, &m_AVIStreamInfo, sizeof(m_AVIStreamInfo));
				if (ret != AVIERR_OK) {
					MessageBox(_T("AVIStreamInfo failed, unable to get stream info!"));
					return;
				}

				if (m_AVIStreamInfo.fccType == streamtypeVIDEO) {						
					LONG lSize;
					ret = AVIStreamReadFormat(m_AVIStream, AVIStreamStart(m_AVIStream), NULL, &lSize);
					if (ret != AVIERR_OK) {
						MessageBox(_T("AVIStreamReadFormat failed, unable to get stream format!"));
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
					CArray<CByteArray, CByteArray&> myList;
					// w00t it's a PNG avi !!!
					CByteArray frameBuffer;
					LONG frameBufferSize = 0;
					LONG frameSample = 0;
					LONG frameLength = AVIStreamLength(m_AVIStream);
					
					CString outputFilenameFormatStr = _T("-%0");			
					// Calc the num of zeros needed
					int zeroCount = 1;
					LONG zeroLength = frameLength;
					while (zeroLength >= 10) {
						zeroLength = zeroLength / 10;
						zeroCount++;
					}
					outputFilenameFormatStr.AppendFormat(_T("%i"), zeroCount);
					outputFilenameFormatStr += _T("i-%s-%s.png");

					LONG samplesRead = 0;			
					while (ret == AVIERR_OK) {				
						ret = AVIStreamRead(m_AVIStream, frameSample, 1, frameBuffer.GetData(), frameBuffer.GetSize(), &frameBufferSize, &samplesRead);
						if (frameBuffer.GetSize() < frameBufferSize) {
							//wxLogDebug(_T("\nFrame Buffer was too small for AVIStreamRead.\n"));
							frameBuffer.SetSize(frameBufferSize, frameBufferSize+1);					
							ret = AVIStreamRead(m_AVIStream, frameSample, AVISTREAMREAD_CONVENIENT, frameBuffer.GetData(), frameBuffer.GetSize(), &frameBufferSize, &samplesRead);
						}				
						if (ret != AVIERR_OK) {
							MessageBox(_T("AVIStreamRead returned error."));
							break;
						}				

						if (frameBufferSize > 0) {							
							if (IsDlgButtonChecked(IDC_RADIO_AVI2PNG)) {
								BYTE *frameData = frameBuffer.GetData();
								LONG frameDataSize = frameBufferSize;
								PNGParser parser(CMemFile(frameData, frameDataSize));
								CString outputName;		
								outputName.Format(outputFilenameFormatStr, frameSample, AVIStreamIsKeyFrame(m_AVIStream, frameSample) ? _T("K") : _T("D"), _T("RGB"));
								outputName = m_OutputFilename + outputName;

								CFile output(outputName, CFile::modeCreate|CFile::modeWrite);
								output.Write(frameData, frameDataSize);	
							} else if (IsDlgButtonChecked(IDC_RADIO_AVI2AVI)) {

								ret = AVIStreamWrite(m_AVIStream, frameSample, 1, frameBuffer.GetData(), frameBuffer.GetSize(), AVIStreamIsKeyFrame(m_AVIStream, frameSample) ? AVIIF_KEYFRAME : 0, &frameBufferSize, &samplesRead);
							}
						}
						int currentPos = (100 / (float)frameLength) * (float)frameSample;
						if (m_ProgressCtrl.GetPos() != currentPos) {
							m_ProgressCtrl.SetPos(currentPos);
							CString progressStr;
							progressStr.Format(_T("Extracting... %02i%%"), currentPos);
							SetDlgItemText(IDC_STATIC_PROGRESS, progressStr);
						}
						{
							MSG msg;
							while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) 
								if (!AfxGetThread()->PumpMessage()) 
									return;

							// let MFC do its idle processing
							LONG lIdle = 0;
							while (AfxGetApp()->OnIdle(lIdle++));  
							if (!m_ButtonStart.IsWindowEnabled()) {
								m_ButtonStart.EnableWindow(TRUE);
								break;
							}
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
		} else if (IsDlgButtonChecked(IDC_RADIO_PNG2AVI)) {
			PAVIFILE outputAVIFile;
			PAVISTREAM outputAVIStream;
			AVISTREAMINFO outputAVIStreamInfo;

			ret = AVIFileOpen(&outputAVIFile, m_OutputFilename, OF_CREATE|OF_WRITE|OF_SHARE_DENY_WRITE, 0L);
			if (ret != AVIERR_OK) {
				CString err;
				err.Format(_T("AVIFileOpen failed, unable to create: '%s'"), m_OutputFilename);
				MessageBox(err);
				return;
			}
			ZeroMemory(&outputAVIStreamInfo, sizeof(AVISTREAMINFO));
			outputAVIStreamInfo.fccType = streamtypeVIDEO;
			outputAVIStreamInfo.fccHandler = MAKE_FOURCC("png1");
			outputAVIStreamInfo.dwLength = m_InputList.GetCount();
			outputAVIStreamInfo.dwQuality	= 10000;
			outputAVIStreamInfo.dwRate = 1000000;
			outputAVIStreamInfo.dwScale = 33366;
			lstrcpy(outputAVIStreamInfo.szName, _T("Video #1"));
			ret = AVIFileCreateStream(outputAVIFile, &outputAVIStream, &outputAVIStreamInfo);
			if (ret != AVIERR_OK) {
				MessageBox(_T("AVIFileCreateStream failed"));
				return;
			}
			ret = AVIStreamSetFormat(outputAVIStream, 0, 0, 0);
		}
	} else {
		// We need to stop the processing
		// Disable this button
		m_ButtonStart.EnableWindow(FALSE);
	}
}

void Cpng2avi2pngDlg::OnBnClickedButtonAdd()
{
	// TODO: Add your control notification handler code here
	if (IsDlgButtonChecked(IDC_RADIO_AVI2PNG) || IsDlgButtonChecked(IDC_RADIO_AVI2AVI)) {
		CFileDialog input(TRUE, 0, 0, OFN_FILEMUSTEXIST, _T("AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||\0"));
		if (input.DoModal() == IDOK) {
			m_InputList.ResetContent();
			m_InputList.AddString(input.GetPathName());
		}
	} else if (IsDlgButtonChecked(IDC_RADIO_PNG2AVI)) {
		CFileDialog input(TRUE, 0, 0, OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_PATHMUSTEXIST, _T("PNG Files (*.png)|*.png|All Files (*.*)|*.*||\0"));
		input.SetOwner(this);
		TCHAR *fileBuffer = new TCHAR[(1024+1)*1024];
		fileBuffer[0] = 0;
		input.m_pOFN->lpstrFile = fileBuffer;
		input.m_pOFN->nMaxFile = 1024*1024;
		if (input.DoModal() == IDOK) {
			CString baseFolder = fileBuffer;
			TCHAR *currentFile = fileBuffer+baseFolder.GetLength()+1;
			while (currentFile[0] != 0 && currentFile[1] != 0) {
				m_InputList.AddString(baseFolder+currentFile);
				currentFile += lstrlen(currentFile);
				currentFile++;				
			}			
		}
		delete [] fileBuffer;
	}
}

void Cpng2avi2pngDlg::OnBnClickedButtonRemove()
{
	// TODO: Add your control notification handler code here
	INT index[1025];
	int indexCount = 0;
	indexCount = m_InputList.GetSelItems(1024, index);
	while (indexCount > 0) {
		m_InputList.DeleteString(index[0]);
		indexCount = m_InputList.GetSelItems(1024, index);
	}
}

void Cpng2avi2pngDlg::OnBnClickedButtonOptions()
{
	// TODO: Add your control notification handler code here
}

void *SwapBytes(void* Addr, const int Nb) {
	static char Swapped[4];
	switch (Nb) {
		case 4:	
			Swapped[0]=*((char*)Addr+3);
			Swapped[1]=*((char*)Addr+2);
			Swapped[2]=*((char*)Addr+1);
			Swapped[3]=*((char*)Addr  );
			break;
	}
	return (void*)Swapped;
}

PNGParser::PNGParser(CFile &input)
{
	width = 0;
	height = 0;
	size = 0;
	// PNG format
	// 0x89 PNG + 4 junk bytes
	// <size<4>> <chunk header<4>> <unknown data<4>> <chunk data<size>>
	long lSize = 0;
	char header[5];
	ZeroMemory(header, 5);
	BYTE buffer[8];	
	input.Read(buffer, 8);
	if (buffer[0] == 0x89 
		&& buffer[1] == 'P' 
		&& buffer[2] == 'N' 
		&& buffer[3] == 'G') 
	{
		bool bEOF = false;
		while (!bEOF) {
			input.Read(buffer, 8);		
			lSize = SWAP_LONG(*((long *)buffer));
			lstrcpynA(header, (LPCSTR)buffer+4, 5);

			if (!lstrcmpiA(header, "IHDR")) {
				input.Read(buffer, 8);
				width = *((long *)buffer);
				width = SWAP_LONG(width);
				height = *(((long *)buffer)+1);
				height = SWAP_LONG(height);
				input.Read(&bitdepth, 1);
				// Seek to the next image chunk
				input.Seek(lSize-9+4, CFile::current);
			} else if (!lstrcmpiA(header, "IEND")) {
				bEOF = true;
				// Seek to the next image
				input.Seek(lSize+4, CFile::current);
			} else {
				// Seek to the next image chunk
				input.Seek(lSize+4, CFile::current);
			}

			if (input.GetPosition() >= input.GetLength())
				bEOF = true;
		}
		size = input.GetPosition();
	}
};
