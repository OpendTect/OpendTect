/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uisrchprocfiles.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "iopar.h"
#include "ioobj.h"
#include "oddirs.h"
#include "dirlist.h"
#include "ctxtioobj.h"
#include "keystrs.h"


uiSrchProcFiles::uiSrchProcFiles( uiParent* p, CtxtIOObj& c, const char* iopky )
    : uiDialog(p,uiDialog::Setup("Find job specification file",
			       "Select appropriate job specification file",
				 mNoHelpID).nrstatusflds(1))
    , ctio_(c)
    , iopkey_(iopky)
{
    ctio_.ctxt.forread = true;

    dirfld = new uiFileInput( this, "Directory to search in",
	    	 uiFileInput::Setup(GetProcFileName(0)).directories(true) );
    maskfld = new uiGenInput( this, "Filename subselection", "*.par" );
    maskfld->attach( alignedBelow, dirfld );
    objfld = new uiIOObjSel( this, ctio_, "Output data to find" );
    objfld->attach( alignedBelow, maskfld );
    objfld->selectionDone.notify( mCB(this,uiSrchProcFiles,srchDir) );
    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, objfld );
    fnamefld = new uiGenInput( this, "-> File name found", FileNameInpSpec(""));
    fnamefld->attach( alignedBelow, objfld );
    fnamefld->attach( ensureBelow, sep );
}


const char* uiSrchProcFiles::fileName() const
{
    return fnamefld->text();
}


#define mRet(s) \
{ \
    if ( s ) uiMSG().error(s); \
    toStatusBar(""); \
    return; \
}

void uiSrchProcFiles::srchDir( CallBacker* )
{
    objfld->commitInput();
    const BufferString key( ctio_.ioobj ? ctio_.ioobj->key().buf() : "" );
    if ( key.isEmpty() ) return;

    uiMsgMainWinSetter msgwinsetter( this );
    	// Otherwise the error box pulls up OD main win. No idea why.

    toStatusBar( "Scanning directory" );
    const BufferString msk( maskfld->text() );
    const BufferString dirnm( dirfld->text() );
    DirList dl( dirnm, DirList::FilesOnly, msk.isEmpty() ? 0 : msk.buf() );
    if ( dl.size() == 0 )
	mRet( "No matching files found" )

    BufferStringSet fnms;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	IOPar iop; const BufferString fnm( dl.fullPath(idx) );
	if ( !iop.read(fnm,sKey::Pars(),true) )
	    continue;
	const char* res = iop.find( iopkey_ );
	if ( res && key == res )
	    fnms.add( fnm );
    }

    if ( fnms.size() < 1 )
	mRet( "Target data not found in files" )
    int sel = 0;
    if ( fnms.size() > 1 )
    {
	toStatusBar( "Multiple files found; select one ..." );
	uiSelectFromList::Setup sflsu( "Select the apropriate file", fnms );
	sflsu.dlgtitle( "Pick one of the matches" );
	uiSelectFromList dlg( this, sflsu );
	if ( !dlg.go() || dlg.selection() < 0 )
	    mRet(0)
	sel = dlg.selection();
    }

    fnamefld->setText( fnms.get(sel) );
    mRet(0)
}


bool uiSrchProcFiles::acceptOK( CallBacker* )
{
    return true;
}
