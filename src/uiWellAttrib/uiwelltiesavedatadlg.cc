/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/


#include "uiwelltiesavedatadlg.h"

#include "createlogcube.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "wavelet.h"
#include "waveletio.h"
#include "wellextractdata.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welllog.h"
#include "welldata.h"
#include "welllogset.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uigeninput.h"
#include "uiwaveletsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

//start at 2, the first 2 are sonic and density.
#define cLogShift	2
#define mErrRet( msg ) { uiMSG().error(msg); return false; }

namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, Server& wdserv )
    : uiDialog( p, uiDialog::Setup(tr("Save current data"),
		tr("Check the items to be saved"),
                mODHelpKey(mWellTieSaveDataDlgHelpID) ) )
    , dataserver_(wdserv)
{
    setCtrlStyle( RunAndClose );
    const Data& data = dataserver_.data();

    uiGroup* loggrp = new uiGroup( this, "Log parameters" );
    logchk_ = new uiCheckBox( loggrp, tr("Log(s)") );
    logchk_->activated.notify( mCB(this,uiSaveDataDlg,saveLogsSelCB) );

    MonitorLock ml( data.logset_ );
    BufferStringSet lognms;
    for ( int idx=cLogShift; idx<data.logset_.size(); idx++)
	lognms.add( data.logset_.getLogByIdx(idx)->name() );
    ml.unlockNow();

    logsfld_ = new uiCheckList( loggrp );
    logsfld_->addItems( lognms );
    logsfld_->attach( alignedBelow, logchk_ );

    saveasfld_ = new uiGenInput( loggrp, uiStrings::sSaveAs(),
				BoolInpSpec( true, uiStrings::sLog(),
					     tr("Seismic cube")) );
    saveasfld_->attach( alignedBelow, logsfld_ );
    saveasfld_->valuechanged.notify(
			mCB(this,uiSaveDataDlg,changeLogUIOutput) );

    outputgrp_ = new uiCreateLogCubeOutputSel( loggrp, true );
    outputgrp_->attach( alignedBelow, saveasfld_ );
    changeLogUIOutput(0);

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, loggrp );

    uiGroup* wvltgrp = new uiGroup( this, "Wavelet parameters" );
    wvltgrp->attach( ensureBelow, horSepar );

    wvltchk_ = new uiCheckBox( wvltgrp, uiStrings::sWavelet() );
    wvltchk_->activated.notify( mCB(this,uiSaveDataDlg,saveWvltSelCB) );

    uiWaveletIOObjSel::Setup su( tr("Initial wavelet") );
    su.optional( true );

    initwvltsel_ = new uiWaveletIOObjSel( wvltgrp, su, false );
    initwvltsel_->setInputText( data.initwvlt_->name() );
    initwvltsel_->attach( alignedBelow, wvltchk_ );

    su.seltxt_ = tr("Estimated wavelet");
    estimatedwvltsel_ = new uiWaveletIOObjSel( wvltgrp, su, false );
    estimatedwvltsel_->setInputText( data.estimatedwvlt_->name() );
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


#define mAppendMsgAndQuit(locmsg,act) { msg.appendPhrase( locmsg ); act; }

bool uiSaveDataDlg::saveLogs()
{
    const Data& data = dataserver_.data();
    const bool savetolog = saveasfld_->getBoolValue();

    Well::LogSet logset;
    BufferStringSet lognms;
    uiString msg;
    MonitorLock ml( data.logset_ );
    for ( int ilog=0; ilog<logsfld_->size(); ilog++ )
    {
	if ( !logsfld_->isChecked(ilog) )
	    continue;

	const Well::Log& log = *data.logset_.getLogByIdx( ilog+cLogShift );
	BufferString lognm( log.name() );
	if ( savetolog )
	    lognm.addSpace().add( outputgrp_->getPostFix() );

	if ( data.wd_->logs().isPresent(lognm) )
	{
	    msg = tr("Log '%1' already exists").arg( lognm );
	    continue;
	}

	Well::Log* newlog = new Well::Log( log );
	newlog->setName( lognm );
	logset.add( newlog );
	lognms.add( lognm );
    }
    ml.unlockNow();

    uiString endmsg = tr("Please choose another postfix");
    if ( !msg.isEmpty() )
	mAppendMsgAndQuit( endmsg, mErrRet( msg ) )

    DataWriter& datawtr = dataserver_.dataWriter();
    endmsg = uiStrings::phrCheckPermissions();
    if ( !datawtr.writeLogs(logset,savetolog) )
    {
	datawtr.removeLogs( logset );
	msg = uiStrings::phrCannotWrite( uiStrings::sLog(logset.size()) );
	mAppendMsgAndQuit( endmsg, mErrRet( msg ) )
    }

    if ( !savetolog )
    {
	const int nrtraces = outputgrp_->getNrRepeatTrcs();
	Well::ExtractParams wep;
	wep.setFixedRange( data.getModelRange(), true );
	LogCubeCreator lcr( lognms, dataserver_.wellID(), wep, nrtraces );
	if ( !lcr.setOutputNm(outputgrp_->getPostFix(),
			      outputgrp_->withWellName()) )
	{
	    if ( !outputgrp_->askOverwrite(lcr.errMsg()) )
	    {
		datawtr.removeLogs( logset );
		return false;
	    }
	    else
		lcr.resetMsg();
	}

	uiTaskRunner* taskrunner = new uiTaskRunner( this );
	if ( !TaskRunner::execute(taskrunner,lcr) || !lcr.isOK() )
	{
	    datawtr.removeLogs( logset );
	    mErrRet( lcr.errMsg() )
	}

	datawtr.removeLogs( logset );
    }

    return true;
}


bool uiSaveDataDlg::saveWvlt( bool useest )
{
    uiWaveletIOObjSel& wvltsel = useest ? *estimatedwvltsel_ : *initwvltsel_;
    if ( !wvltsel.isChecked() )
	return true;

    const Data& data = dataserver_.data();
    const Wavelet& wvlt = useest ? *data.estimatedwvlt_ : *data.initwvlt_;
    if ( !wvlt.size() && useest )
    {
	uiString msg = tr("No estimated wavelet yet");
	msg.appendPhrase(
	   tr("Press 'Display additional information' before saving") );
	mErrRet( msg )
    }

    return wvltsel.store( wvlt );
}


bool uiSaveDataDlg::acceptOK()
{
    bool success = true;

    if ( logsfld_->firstChecked() == -1 && !initwvltsel_->isChecked() &&
	 !estimatedwvltsel_->isChecked() )
	mErrRet( tr("Check at least one item to be saved") )

    if ( !saveLogs() || !saveWvlt(false) || !saveWvlt(true) )
	success = false;

    if ( success )
	uiMSG().message( tr("Successfully saved the selected items") );

    return false;
}

} // namespace WellTie
