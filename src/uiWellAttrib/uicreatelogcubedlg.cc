/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicreatelogcubedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitaskrunner.h"

#include "createlogcube.h"
#include "multiid.h"
#include "od_helpids.h"


uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const MultiID* mid )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
				 "Select logs to create new cubes",
				 mid ? mODHelpKey(mCreateLogCubeDlgHelpID)
				     : mODHelpKey(mMultiWellCreateLogCubeDlg) ))
{
    setCtrlStyle( RunAndClose );

    uiWellExtractParams::Setup su;
    su.withzstep(false).withsampling(true).withextractintime(false);
    welllogsel_ = mid ? new uiMultiWellLogSel( this, su, *mid )
		      : new uiMultiWellLogSel( this, su );

    outputgrp_ = new uiCreateLogCubeOutputSel( this, false );
    outputgrp_->attach( alignedBelow, welllogsel_ );
}


#define mErrRet( msg, act ) { uiMSG().error( msg ); act; }
bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const Well::ExtractParams& extractparams = welllogsel_->params();
    const int nrtrcs = outputgrp_->getNrRepeatTrcs() + 1;

    TypeSet<MultiID> wids;
    welllogsel_->getSelWellIDs( wids );
    if ( wids.isEmpty() )
	mErrRet("No well selected",return false);

    if ( wids.size() > 1 && !outputgrp_->withWellName()  )
	mErrRet("Multiple wells selected, "
		"output name must contain the well name",return false);

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

    uiTaskRunner* tr = new uiTaskRunner( this );
    if ( !TaskRunner::execute(tr,lcr) || lcr.errMsg() )
	mErrRet( lcr.errMsg(), return false );

    uiMSG().message( "Successfully created the log cube(s)" );

    return false;
}



uiCreateLogCubeOutputSel::uiCreateLogCubeOutputSel( uiParent* p, bool withmerge)
    : uiGroup(p,"Create LogCube output specification Group")
    , domergefld_(0)
{
    repeatfld_ = new uiLabeledSpinBox( this,"Duplicate trace around the track");
    repeatfld_->box()->setInterval( 0, 40, 1 );
    repeatfld_->box()->setValue( 1 );

    uiSeparator* sep = new uiSeparator( this, "Save Separ" );
    sep->attach( stretchedBelow, repeatfld_ );

    uiGroup* outputgrp = new uiGroup( this, "Output name group" );
    outputgrp->attach( ensureBelow, sep );

    uiLabel* savelbl = new uiLabel( outputgrp, "Output name" );
    savewllnmfld_ = new uiCheckBox( outputgrp, "with well name" );
    savewllnmfld_->setChecked( true );
    savewllnmfld_->attach( rightOf, savelbl );

    savepostfix_ = new uiGenInput( outputgrp, "Postfix", "log cube" );
    savepostfix_->attach( rightOf, savewllnmfld_ );

    if ( !withmerge )
	return;

    domergefld_ = new uiCheckBox( this, "Keep individual volumes" );
    domergefld_->attach( alignedBelow, outputgrp );
}


const char* uiCreateLogCubeOutputSel::getPostFix() const
{
    return savepostfix_->text();
}


bool uiCreateLogCubeOutputSel::withWellName() const
{
    return savewllnmfld_->isChecked();
}


int uiCreateLogCubeOutputSel::getNrRepeatTrcs() const
{
    return repeatfld_->box()->getValue();
}


void uiCreateLogCubeOutputSel::setPostFix( const BufferString& nm )
{
    savepostfix_->setText( nm );
}


void uiCreateLogCubeOutputSel::useWellNameFld( bool use )
{
    savewllnmfld_->display( use );
    savewllnmfld_->setChecked( use );
}


void uiCreateLogCubeOutputSel::displayRepeatFld( bool disp )
{
    repeatfld_->display( disp );
}


bool uiCreateLogCubeOutputSel::askOverwrite( BufferString errmsg ) const
{
    if ( errmsg.find("as another type") )
    {
	errmsg.addNewLine().add( "Please choose another postfix" );
	mErrRet( errmsg, return false );
    }

    errmsg.addNewLine().add( "Overwrite?" );
    return uiMSG().askOverwrite( errmsg );
}

