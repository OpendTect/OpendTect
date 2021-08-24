/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/

#include "uicreatelogcubedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uistring.h"

#include "createlogcube.h"
#include "multiid.h"
#include "od_helpids.h"


// uiCreateLogCubeDlg
uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const MultiID* key )
    : uiDialog(p,uiDialog::Setup(tr("Create Log Cube"),
				 tr("Select logs to create new cubes"),
				 key ? mODHelpKey(mCreateLogCubeDlgHelpID)
				     : mODHelpKey(mMultiWellCreateLogCubeDlg) ))
    , key_(key ? *key : MultiID::udf())
{
    setCtrlStyle( RunAndClose );

    const bool validinpwell = key && !key->isUdf();
    uiWellExtractParams::Setup su;
    su.withzstep(false).withsampling(true).withextractintime(false);
    welllogsel_ = validinpwell ? new uiMultiWellLogSel( this, su, *key )
			       : new uiMultiWellLogSel( this, su );

    outputgrp_ = new uiCreateLogCubeOutputSel( this, validinpwell );
    outputgrp_->attach( alignedBelow, welllogsel_ );
}


#define mErrRet( msg ) { uiMSG().error( msg ); return false; }


bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const Well::ExtractParams& extractparams = welllogsel_->params();
    const int nrtrcs = outputgrp_->getNrRepeatTrcs();

    TypeSet<MultiID> wids;
    welllogsel_->getSelWellIDs( wids );
    if ( wids.isEmpty() )
	mErrRet(tr("No well selected") );

    BufferStringSet lognms;
    welllogsel_->getSelLogNames( lognms );
    LogCubeCreator lcr( lognms, wids, extractparams, nrtrcs );
    if ( !lcr.setOutputNm(outputgrp_->getPostFix(),outputgrp_->withWellName()) )
    {
	if ( !outputgrp_->askOverwrite(lcr.errMsg()) )
	    return false;
	else
	    lcr.resetMsg();
    }

    uiTaskRunner* taskrunner = new uiTaskRunner( this );
    if ( !TaskRunner::execute(taskrunner,lcr) || !lcr.isOK() )
	mErrRet( lcr.errMsg() )

    BufferStringSet outputnames;
    lcr.getOutputNames( outputnames );
    uiMSG().message( tr("Successfully created log cube(s):\n%1")
			.arg(outputnames.cat()) );

    return false;
}


// uiCreateLogCubeOutputSel
uiCreateLogCubeOutputSel::uiCreateLogCubeOutputSel( uiParent* p, bool withwllnm)
    : uiGroup(p,"Create LogCube output specification Group")
    , savewllnmfld_(0)
{
    repeatfld_ = new uiLabeledSpinBox( this,
				tr("Duplicate trace around the track") );
    repeatfld_->box()->setInterval( 0, 20, 1 );
    repeatfld_->box()->setValue( 1 );

    uiLabel* savelbl = new uiLabel( this,
			uiStrings::phrOutput(uiStrings::sName()) );
    savelbl->attach( leftAlignedBelow, repeatfld_ );
    savesuffix_ = new uiGenInput( this, tr("with suffix"), "log cube" );
    savesuffix_->setWithCheck( true );
    savesuffix_->setChecked( true );
    savesuffix_->attach( rightOf, savelbl );

    if ( withwllnm )
    {
	savewllnmfld_ = new uiCheckBox( this, tr("With well name") );
	savewllnmfld_->setChecked( true );
	savewllnmfld_->attach( rightOf, savesuffix_ );
    }
}


int uiCreateLogCubeOutputSel::getNrRepeatTrcs() const
{
    return repeatfld_->box()->getIntValue();
}


const char* uiCreateLogCubeOutputSel::getPostFix() const
{
    if ( !savesuffix_->isChecked() )
	return 0;

    return savesuffix_->text();
}


bool uiCreateLogCubeOutputSel::withWellName() const
{
    return savewllnmfld_ && savewllnmfld_->isChecked();
}


void uiCreateLogCubeOutputSel::displayRepeatFld( bool disp )
{
    repeatfld_->display( disp );
}


void uiCreateLogCubeOutputSel::setPostFix( const BufferString& nm )
{
    savesuffix_->setText( nm );
}


void uiCreateLogCubeOutputSel::useWellNameFld( bool disp )
{
    if ( !savewllnmfld_ )
	return;

    savewllnmfld_->display( disp );
    savewllnmfld_->setChecked( disp );
}


bool uiCreateLogCubeOutputSel::askOverwrite( const uiString& errmsg ) const
{
    if ( BufferString(errmsg.getFullString()).find("as another type") )
    {
	uiString msg( errmsg );
	msg.append( tr( "Please choose another suffix" ), true );
	mErrRet( msg )
    }

    return uiMSG().askOverwrite( errmsg );
}
