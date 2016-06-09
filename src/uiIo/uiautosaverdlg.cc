/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiautosaverdlg.h"
#include "autosaver.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "od_helpids.h"
#include "separstr.h"
#include "settings.h"

static const char* savemodes[] = { "Keep emergency copies", "Save", 0 };


/*\brief Allows user to create proper objects from auto-saved copies */

class AutoSaved2RealObjectRestorer : public CallBacker
{
public:

AutoSaved2RealObjectRestorer()
{
    doWork( 0 );
    mAttachCB( IOM().surveyChanged, AutoSaved2RealObjectRestorer::doWork );
}

void doWork( CallBacker* )
{
    ObjectSet<IOObj> ioobjs;
    IOObjSelConstraints cnstrts;
    cnstrts.allownonuserselectable_ = true;
    cnstrts.require_.set( sKey::CrInfo(), "Auto-saved" );
    IOM().findTempObjs( ioobjs, &cnstrts );
    if ( ioobjs.isEmpty() )
	return;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj& ioobj = *ioobjs[idx];
	FixedString crfrom = ioobj.pars().find( sKey::CrFrom() );
	BufferString orgnm( "<No name>" );
	if ( !crfrom.isEmpty() )
	{
	    FileMultiString fms( crfrom );
	    BufferString nm( fms[1] );
	    if ( !nm.isEmpty() )
		orgnm.set( nm );
	}
	uiMSG().warning( toUiString(
	    BufferString("TODO: handle auto-saved copy of: ").add( orgnm ) ) );

	// IOM().commitChanges( ioobj );
    }

    deepErase( ioobjs );
}

}; // class AutoSaved2RealObjectRestorer

static AutoSaved2RealObjectRestorer* autosaved_2_realobj_restorer = 0;

void startAutoSaved2RealObjectRestorer()
{
    autosaved_2_realobj_restorer = new AutoSaved2RealObjectRestorer;
}



uiAutoSaverDlg::uiAutoSaverDlg( uiParent* p )
	: uiDialog(p,uiDialog::Setup(tr("Auto-Saver settings"),mNoDlgTitle,
				      mTODOHelpKey))
{
    const OD::AutoSaver& autosaver = OD::AUTOSAVE();

    isactivefld_ = new uiGenInput( this, tr("Auto-Save precious data"),
			    BoolInpSpec(autosaver.isActive()) );
    isactivefld_->valuechanged.notify( mCB(this,uiAutoSaverDlg,isActiveCB) );

    StringListInpSpec savemodespec( savemodes );
    savemodespec.setValue( autosaver.useHiddenMode() ? 0 : 1 );
    usehiddenfld_ = new uiGenInput( this, tr("Auto-Save mode"),
			    StringListInpSpec(savemodes) );
    usehiddenfld_->attach( alignedBelow, isactivefld_ );

    nrsecondsfld_ = new uiGenInput( this, tr("Save interval (seconds)"),
			    IntInpSpec(autosaver.nrSecondsBetweenSaves()) );
    nrsecondsfld_->attach( alignedBelow, usehiddenfld_ );

    postFinalise().notify( mCB(this,uiAutoSaverDlg,isActiveCB) );
}


void uiAutoSaverDlg::isActiveCB( CallBacker* )
{
    const bool isactive = isactivefld_->getBoolValue();
    usehiddenfld_->display( isactive );
    nrsecondsfld_->display( isactive );
}


#define mErrRet(s) { uiMSG().error(s); return; }

bool uiAutoSaverDlg::acceptOK( CallBacker* )
{
    OD::AutoSaver& autosaver = OD::AUTOSAVE();

    const bool isactive = isactivefld_->getBoolValue();
    OD::AUTOSAVE().setActive( isactive );
    if ( isactive )
    {
	autosaver.setUseHiddenMode( usehiddenfld_->getIntValue() == 0 );
	autosaver.setNrSecondsBetweenSaves( nrsecondsfld_->getIntValue() );
    }

    Settings::common().write();
    return true;
}
