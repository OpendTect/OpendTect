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
#include "od_helpids.h"
#include "settings.h"

static const char* savemodes[] = { "Keep emergency copies", "Save", 0 };
static const char* sKeyAutoAskRestore = "AutoSave.Confront Restore";

bool uiAutoSaverDlg::autoAskRestore()
{
    bool yn = true;
    Settings::common().getYN( sKeyAutoAskRestore, yn );
    return yn;
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

    autoaskfld_ = new uiGenInput( this,
			tr("Present recoverable objects at startup"),
			BoolInpSpec(autoAskRestore()) );
    autoaskfld_->attach( alignedBelow, nrsecondsfld_ );

    postFinalise().notify( mCB(this,uiAutoSaverDlg,isActiveCB) );
}


void uiAutoSaverDlg::isActiveCB( CallBacker* )
{
    const bool isactive = isactivefld_->getBoolValue();
    usehiddenfld_->display( isactive );
    nrsecondsfld_->display( isactive );
}


bool uiAutoSaverDlg::acceptOK()
{
    OD::AutoSaver& autosaver = OD::AUTOSAVE();

    const bool isactive = isactivefld_->getBoolValue();
    OD::AUTOSAVE().setActive( isactive );
    if ( isactive )
    {
	autosaver.setUseHiddenMode( usehiddenfld_->getIntValue() == 0 );
	autosaver.setNrSecondsBetweenSaves( nrsecondsfld_->getIntValue() );
    }
    if ( autoaskfld_->getBoolValue() )
	Settings::common().setYN( sKeyAutoAskRestore, true );
    else
	Settings::common().removeWithKey( sKeyAutoAskRestore );

    Settings::common().write();
    return true;
}
