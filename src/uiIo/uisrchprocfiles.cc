/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uisrchprocfiles.h"

#include "uifilesel.h"
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
    : uiDialog(p,uiDialog::Setup(tr("Find job specification file"),
			       tr("Select appropriate job specification file"),
				 mNoHelpKey).nrstatusflds(1))
    , ctio_(c)
    , iopkey_(iopky)
{
    ctio_.ctxt_.forread_ = true;

    uiFileSel::Setup fssu( GetProcFileName(0) );
    fssu.selectDirectory();
    dirfld = new uiFileSel( this, tr("Directory to search in"), fssu );
    maskfld = new uiGenInput( this, tr("Filename subselection"), "*.par" );
    maskfld->attach( alignedBelow, dirfld );
    objfld = new uiIOObjSel( this, ctio_, uiStrings::phrOutput(tr(
							    "data to find")) );
    objfld->attach( alignedBelow, maskfld );
    objfld->selectionDone.notify( mCB(this,uiSrchProcFiles,srchDir) );
    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, objfld );
    fnamefld = new uiGenInput( this, tr("-> File name found"),
                               FileNameInpSpec(""));
    fnamefld->attach( alignedBelow, objfld );
    fnamefld->attach( ensureBelow, sep );
}


const char* uiSrchProcFiles::fileName() const
{
    return fnamefld->text();
}


#define mRet(s) \
{ \
    if ( !s.isEmpty() ) uiMSG().error(s); \
    toStatusBar( uiString::empty() ); \
    return; \
}

void uiSrchProcFiles::srchDir( CallBacker* )
{
    objfld->commitInput();
    if ( !ctio_.ioobj_ )
	return;
    const BufferString key( ctio_.ioobj_->key().toString() );
    if ( key.isEmpty() )
	return;

    toStatusBar( tr("Scanning Directory") );
    const BufferString msk( maskfld->text() );
    const BufferString dirnm( dirfld->text() );
    DirList dl( dirnm, File::FilesInDir, msk.isEmpty() ? 0 : msk.buf() );
    if ( dl.size() == 0 )
	mRet(uiStrings::phrCannotFind(uiStrings::sFile()))

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
	mRet(tr("Target data not found in files"))
    int sel = 0;
    if ( fnms.size() > 1 )
    {
	toStatusBar( tr("Multiple files found; select one ...") );
	uiSelectFromList::Setup sflsu( tr("Select the apropriate file"), fnms );
	sflsu.dlgtitle( tr("Pick one of the matches") );
	uiSelectFromList dlg( this, sflsu );
	if ( !dlg.go() || dlg.selection() < 0 )
	    mRet(uiString::empty())
	sel = dlg.selection();
    }

    fnamefld->setText( fnms.get(sel) );
    mRet(uiString::empty())
}


bool uiSrchProcFiles::acceptOK()
{
    return true;
}
