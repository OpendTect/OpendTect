/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimfcdialog.h"

CWinApp theApp;

bool initMFCDialog( HWND hwnd )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWnd* parent = CWnd::FromHandle( hwnd );
    uiMFCDialog* dialog = new uiMFCDialog;
    dialog->Create( IDD_DIALOG1, parent );  
    return dialog->ShowWindow( SW_SHOW );
}


IMPLEMENT_DYNAMIC(uiMFCDialog, CDialog)

uiMFCDialog::uiMFCDialog( CWnd* parent )
	: CDialog(uiMFCDialog::IDD, parent)
	, mesgtext(_T(""))
{

}

uiMFCDialog::~uiMFCDialog()
{
}

void uiMFCDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT1, mesgtext);
}


BEGIN_MESSAGE_MAP(uiMFCDialog, CDialog)
    ON_BN_CLICKED(IDOK, &uiMFCDialog::OnBnClickedOk)
END_MESSAGE_MAP()



void uiMFCDialog::OnBnClickedOk()
{
   UpdateData( true );
   MessageBox( mesgtext );
   return;
   OnOK();
}
