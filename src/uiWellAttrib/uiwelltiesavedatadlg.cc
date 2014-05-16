/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltiesavedatadlg.h"

#include "createlogcube.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "wavelet.h"
#include "wellextractdata.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welllog.h"
#include "welldata.h"
#include "welllogset.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

//start at 2, the first 2 are sonic and density.
#define cLogShift	2
#define mErrRet(msg,act) { uiMSG().error(msg); act; }
namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, Server& wdserv )
    : uiDialog( p, uiDialog::Setup("Save current data",
		"Check the items to be saved",
                mODHelpKey(mWellTieSaveDataDlgHelpID) ) )
    , dataserver_(wdserv)
{
    setCtrlStyle( RunAndClose );
    const Data& data = dataserver_.data();

    uiGroup* loggrp = new uiGroup( this, "Log parameters" );
    logchk_ = new uiCheckBox( loggrp, "Log(s)" );
    logchk_->activated.notify( mCB(this,uiSaveDataDlg,saveLogsSelCB) );

    BufferStringSet lognms;
    for ( int idx=cLogShift; idx<data.logset_.size(); idx++)
	lognms.add( data.logset_.getLog(idx).name() );

    logsfld_ = new uiCheckList( loggrp );
    logsfld_->addItems( lognms );
    logsfld_->attach( alignedBelow, logchk_ );

    saveasfld_ = new uiGenInput( loggrp, "Save as",
				BoolInpSpec( true, "Log", "Seismic cube") );
    saveasfld_->attach( alignedBelow, logsfld_ );
    saveasfld_->valuechanged.notify(
			mCB(this,uiSaveDataDlg,changeLogUIOutput) );

    outputgrp_ = new uiCreateLogCubeOutputSel( loggrp, false );
    outputgrp_->attach( alignedBelow, saveasfld_ );
    changeLogUIOutput(0);

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, loggrp );

    uiGroup* wvltgrp = new uiGroup( this, "Wavelet parameters" );
    wvltgrp->attach( ensureBelow, horSepar );

    wvltchk_ = new uiCheckBox( wvltgrp, "Wavelet" );
    wvltchk_->activated.notify( mCB(this,uiSaveDataDlg,saveWvltSelCB) );

    IOObjContext ctxt = mIOObjContext(Wavelet);
    ctxt.forread = false;
    uiIOObjSel::Setup su( "Initial wavelet" ); su.optional( true );

    initwvltsel_ = new uiIOObjSel( wvltgrp, ctxt, su );
    initwvltsel_->setInputText( data.initwvlt_.name() );
    initwvltsel_->attach( alignedBelow, wvltchk_ );

    su.seltxt_ = "Estimated wavelet";
    estimatedwvltsel_ = new uiIOObjSel( wvltgrp, ctxt, su );
    estimatedwvltsel_->setInputText( data.estimatedwvlt_.name() );
    estimatedwvltsel_->attach( alignedBelow, initwvltsel_ );
}


void uiSaveDataDlg::changeLogUIOutput( CallBacker* )
{
    const bool islogcube = !saveasfld_->getBoolValue();
    outputgrp_->setPostFix( islogcube ? "log cube": "from well tie" );
    outputgrp_->useWellNameFld( islogcube );
    outputgrp_->displayRepeatFld( islogcube );
}


void uiSaveDataDlg::saveLogsSelCB( CallBacker* )
{
    const bool saveall = logchk_->isChecked();
    for ( int ilog=0; ilog<logsfld_->size(); ilog++ )
	logsfld_->setChecked( ilog, saveall );
}


void uiSaveDataDlg::saveWvltSelCB( CallBacker* )
{
    const bool saveall = wvltchk_->isChecked();
    initwvltsel_->setChecked( saveall );
    estimatedwvltsel_->setChecked( saveall );
}


#define mCanNotWriteLogs(iscube)\
{\
    msg.set( dataserver_.dataWriter().errMsg() );\
    msg.add( "Cannot write log" );\
    if ( iscube ) msg.add( " cube" );\
    msg.add("(s)");\
    msg.addNewLine().add("Check your permissions");\
    mErrRet( msg, return false );\
}


bool uiSaveDataDlg::saveLogs()
{
    const Data& data = dataserver_.data();
    const bool savetolog = saveasfld_->getBoolValue();

    Well::LogSet logset;
    BufferStringSet lognms;
    BufferString msg;
    for ( int ilog=0; ilog<logsfld_->size(); ilog++ )
    {
	if ( !logsfld_->isChecked(ilog) )
	    continue;

	const Well::Log& log = data.logset_.getLog( ilog+cLogShift );
	BufferString lognm( log.name() );
	if ( savetolog )
	    lognm.addSpace().add( outputgrp_->getPostFix() );

	if ( data.wd_->logs().getLog(lognm) )
	{
	    if ( !msg.isEmpty() ) msg.addNewLine();
	    msg.add( "Log: '" ).add( lognm ).add( "' already exists " );
	    continue;
	}

	Well::Log* newlog = new Well::Log( log );
	newlog->setName( lognm );
	logset.add( newlog );
	lognms.add( lognm );
    }

    if ( !msg.isEmpty() )
    {
	msg.addNewLine().add( "Please choose another postfix" );
	mErrRet( msg, return false );
    }

    BufferString errmsg( "Can not write " );
    DataWriter& datawtr = dataserver_.dataWriter();
    if ( !datawtr.writeLogs(logset,savetolog) )
    {
	datawtr.removeLogs( logset );
	mCanNotWriteLogs(false);
    }

    if ( !savetolog )
    {
	const int nrtraces = outputgrp_->getNrRepeatTrcs() + 1;
	Well::ExtractParams wep;
	wep.setFixedRange( data.getModelRange(), true );
	LogCubeCreator lcr( lognms, dataserver_.wellID(), wep, nrtraces );
	if ( !lcr.setOutputNm(outputgrp_->getPostFix(),
			      outputgrp_->withWellName()) )
	{
	    if ( !outputgrp_->askOverwrite(lcr.errMsg()) )
	    {
		mCanNotWriteLogs(true);
	    }
	    else
		lcr.resetMsg();
	}

	uiTaskRunner* tr = new uiTaskRunner( this );
	if ( !TaskRunner::execute(tr,lcr) || lcr.errMsg() )
	{
	    datawtr.removeLogs( logset );
	    mErrRet( lcr.errMsg(), return false );
	}

	datawtr.removeLogs( logset );
    }

    return true;
}


bool uiSaveDataDlg::saveWvlt( bool isestimated )
{
    uiIOObjSel& wvltsel = isestimated ? *estimatedwvltsel_ : *initwvltsel_;
    if ( !wvltsel.isChecked() )
	return true;

    const Data& data = dataserver_.data();
    const Wavelet& wvlt = isestimated ? data.estimatedwvlt_ : data.initwvlt_;
    if ( !wvlt.size() && isestimated )
    {
	BufferString msg( "No estimated wavelet yet" );
	msg.addNewLine();
	msg.add( "Press 'Display additional information' before saving" );
	mErrRet( msg, return false );
    }

    const IOObj* wvltioobj = wvltsel.ioobj();
    if ( !wvltioobj )
	return false;

    if ( !wvlt.put(wvltioobj) )
	mErrRet( "Cannot write output wavelet", return false );

    return true;
}


bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    bool success = true;
    if ( logsfld_->firstChecked() == -1 && !initwvltsel_->isChecked() &&
	 !estimatedwvltsel_->isChecked() )
	mErrRet( "Please check at least one item to be saved", return false );

    if ( !saveLogs() )
	success = false;

    if ( !saveWvlt(false) || !saveWvlt(true) )
	success = false;

    if ( success )
	uiMSG().message( "Successfully saved the selected items" );

    return false;
}

}; //namespace Well Tie

