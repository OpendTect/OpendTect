/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiautosavesettings.h"
#include "autosaver.h"
#include "uigeninput.h"
#include "od_helpids.h"
#include "uistrings.h"
#include "settings.h"

static const char* sKeyAutoAskRestore = "AutoSave.Confront Restore";

bool uiAutoSaverSettingsGroup::autoAskRestore()
{
    bool yn = true;
    Settings::common().getYN( sKeyAutoAskRestore, yn );
    return yn;
}


uiAutoSaverSettingsGroup::uiAutoSaverSettingsGroup( uiParent* p,
						    Settings& setts )
    : uiSettingsGroup(p,setts)
    , wasactive_(OD::AUTOSAVE().isActive())
    , washidden_(OD::AUTOSAVE().useHiddenMode())
    , oldnrsecs_(OD::AUTOSAVE().nrSecondsBetweenSaves())
{
    isactivefld_ = new uiGenInput( this, tr("Auto-Save precious data"),
			    BoolInpSpec(wasactive_) );
    mAttachCB( isactivefld_->valuechanged,
		uiAutoSaverSettingsGroup::isActiveCB );

    usehiddenfld_ = new uiGenInput( this, tr("Auto-Save mode"),
			    BoolInpSpec(washidden_,
			    tr("Keep emergency copies"),uiStrings::sSave()) );
    usehiddenfld_->attach( alignedBelow, isactivefld_ );

    nrsecondsfld_ = new uiGenInput( this, tr("Save interval (seconds)"),
				    IntInpSpec(oldnrsecs_,10,3600) );
    nrsecondsfld_->attach( alignedBelow, usehiddenfld_ );

    autoaskfld_ = new uiGenInput( this,
			tr("Present recoverable objects at startup"),
			BoolInpSpec(autoAskRestore()) );
    autoaskfld_->attach( alignedBelow, nrsecondsfld_ );

    postFinalise().notify( mCB(this,uiAutoSaverSettingsGroup,isActiveCB) );

    bottomobj_ = autoaskfld_;
}


void uiAutoSaverSettingsGroup::isActiveCB( CallBacker* )
{
    const bool isactive = isactivefld_->getBoolValue();
    usehiddenfld_->display( isactive );
    nrsecondsfld_->display( isactive );
}


void uiAutoSaverSettingsGroup::doCommit( uiRetVal& )
{
    OD::AutoSaver& autosaver = OD::AUTOSAVE();

    const bool isactive = isactivefld_->getBoolValue();
    if ( isactive )
    {
	const bool ishidden = usehiddenfld_->getBoolValue();
	if ( washidden_ != ishidden )
	{
	    changed_ = true;
	    autosaver.setUseHiddenMode( ishidden );
	}

	const int newnrsecs = nrsecondsfld_->getIntValue();
	if ( oldnrsecs_ != newnrsecs )
	{
	    changed_ = true;
	    autosaver.setNrSecondsBetweenSaves( nrsecondsfld_->getIntValue() );
	}
    }

    const bool wasautoask = autoAskRestore();
    const bool isautoask = autoaskfld_->getBoolValue();
    if ( wasautoask != isautoask )
    {
	changed_ = true;
	Settings::common().setYN( sKeyAutoAskRestore, isautoask );
    }

    if ( wasactive_ != isactive )
    {
	changed_ = true;
	autosaver.setActive( isactive );
    }
}
