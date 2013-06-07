// RSMDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RSM.h"
#include "RSMDlg.h"
#include "TrayIcon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
unsigned int PID = 0;

// CAboutDlg dialog used for App About


int StartProg( const char* comm );
int StopProg();
const char* GetServiceFile();


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


// CRSMDlg dialog




CRSMDlg::CRSMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRSMDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	trayicon_ =  new TrayIcon();
}

void CRSMDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, loglist_);
}

BEGIN_MESSAGE_MAP(CRSMDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_TRAYNOTIFY, OnTrayNotify)
	ON_COMMAND( ID_SHOW, OnShowdialog)
	ON_COMMAND(ID_EXIT, OnMenuExit)
	ON_COMMAND( ID_MENU_START,OnMenuStart )
	ON_COMMAND( ID_MENU_STOP, OnMenuStop )
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_START, &CRSMDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CRSMDlg::OnBnClickedButtonStop)
END_MESSAGE_MAP()


// CRSMDlg message handlers

#define mErrMsg(s) { LogMessage(s); MessageBox( s, s, MB_ICONERROR ); return; } 
BOOL CRSMDlg::OnInitDialog()
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
	trayicon_->SetIcon( m_hIcon  );
	trayicon_->Show();
	trayicon_->SetTooltip( L"Running OpendTect Multi-Machine Service" );
	startUP();
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CRSMDlg::startUP()
{
    OnMenuStart();
   // OnSize( SW_HIDE, 100, 100);
    //ShowWindow( SW_HIDE );
}


void CRSMDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CRSMDlg::OnPaint()
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
HCURSOR CRSMDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CRSMDlg::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{  
    switch ( lParam )
    {
    case WM_RBUTTONUP:
        {   CMenu temp;
            CMenu *popup;
            CPoint loc;
            GetCursorPos( &loc );
            temp.LoadMenu( IDR_TRAY );
            popup = temp.GetSubMenu( 0 );
            popup->TrackPopupMenu( TPM_LEFTALIGN, loc.x, loc.y, this );
        }
        break;
    default:
        break;
    }
    return 0;
}


void CRSMDlg::OnShowdialog() 
{
    SetForegroundWindow();
    ShowWindow( SW_SHOW );
    if ( IsIconic() )
	ShowWindow( SW_RESTORE );
}


void CRSMDlg::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize( nType, cx, cy );
    if( nType == SIZE_MINIMIZED)
    	ShowWindow( SW_HIDE );	
}


void CRSMDlg::OnMenuExit() 
{
    StopProg();
    trayicon_->Hide();
    PostMessage( WM_QUIT );		
}


void CRSMDlg::OnMenuStart()
{
    if ( PID != 0 )
    {
	MessageBox( L"Service Runnig" );
	return;
    }
    
    /*if ( !StartProg( GetServiceFile() ) )
	mErrMsg(L"Start Service Failed")*/
    
    LogMessage( L"Service Started" );
    trayicon_->SetIcon( AfxGetApp()->LoadIcon(IDR_ICON_START) );
}


void CRSMDlg::OnMenuStop()
{   
    if ( !StopProg() )
	mErrMsg(L"Stop Service Failed")

    LogMessage( L"Service Stopped" );
    trayicon_->SetIcon( AfxGetApp()->LoadIcon(IDI_ICON_STOP) );
}


void CRSMDlg::LogMessage( CString msg )
{
    CTime time = time.GetCurrentTime();
    int dd, mm, yy, h, m, s;
    h=time.GetHour();
    m=time.GetMinute();
    s=time.GetSecond();
    dd=time.GetDay();
    mm=time.GetMonth();
    yy=time.GetYear();
				
    CString timemsg;
    timemsg.Format(L" On %0.2d/%0.2d/%0.2d At %0.2d:%0.2d:%0.2d",dd,mm,yy,h,m,s);
    msg += timemsg;	
    loglist_.AddString( msg );
    const int sel = loglist_.GetCount();
    loglist_.SetCurSel( sel-1 );
}

static char currentdir[1024];

int StartProg( const char* comm )
{
    if ( !comm || !*comm ) return false;
    
   /* CRSMDlg* dlg = (CRSMDlg*) theApp.m_pMainWnd;
    CString com( "Comm " );
    com += comm;
    dlg->LogMessage( com );
    CString curdir ( "Current dir " );
    curdir += currentdir;

    dlg->LogMessage( curdir );*/

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	
    si.wShowWindow = SW_HIDE;
   
    int res = CreateProcessA( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        0,				// Creation flags. 
        NULL,				// Use parent's environment block. 
        const_cast<char*>( currentdir ), // Use parent's starting directory. 
        &si, &pi );

    if ( res )
    {
	PID = pi.dwProcessId;
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;
}


int StopProg()
{
    HANDLE hproc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, PID );
    int ret = TerminateProcess( hproc, 0 );
    PID = 0;
    return ret;
}


bool file_Exists( CString fnm )
{
    WIN32_FIND_DATA findData;
    ZeroMemory( &findData, sizeof(findData) );
    HANDLE handle = FindFirstFile( fnm, &findData );
    return handle == INVALID_HANDLE_VALUE ? false : true;
}

const char* GetServiceFile()
{
    char path[1024];
    GetModuleFileNameA(NULL, path, (sizeof(path)/sizeof(char)));
    const char* bin32 = "bin\\win32";
    const char* bin64 = "bin\\win64";
    int len = strlen( path ) - strlen( "bin\\win32\\rsm\\od_remote_service_manager.exe" );
    path[len] = '\0';
    static char serverpath[1024];
    sprintf( currentdir, "%s\%s", path, bin32 );
    sprintf( serverpath,"%s\\od_remoteservice.exe", currentdir );
    if ( file_Exists( CString(serverpath) ) )
	return serverpath;
    sprintf( currentdir, "%s\%s", path, bin64 );
    sprintf( serverpath,"%s\\od_remoteservice.exe", currentdir );
    return serverpath;
}


void CRSMDlg::OnClose()
{
    SendMessage( WM_SIZE, SIZE_MINIMIZED );
}


void CRSMDlg::OnBnClickedButtonStart()
{
    OnMenuStart();
}

void CRSMDlg::OnBnClickedButtonStop()
{
    OnMenuStop();
}
