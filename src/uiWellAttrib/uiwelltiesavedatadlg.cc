/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelltiesavedatadlg.h"

#include "createlogcube.h"
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
#include "uiioobjsel.h"
#include "uilabel.h"
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
    : uiDialog(p,uiDialog::Setup(tr("Save current data"),
			tr("Check the items to be saved"),
			mODHelpKey(mWellTieSaveDataDlgHelpID)))
    , dataserver_(wdserv)
{
    setOkCancelText( uiStrings::sSave(), uiStrings::sClose() );
    const Data& data = dataserver_.data();

    uiGroup* loggrp = new uiGroup( this, "Log parameters" );
    logchk_ = new uiCheckBox( loggrp, tr("Log(s)") );
    logchk_->activated.notify( mCB(this,uiSaveDataDlg,saveLogsSelCB) );

    BufferStringSet lognms;
    for ( int idx=cLogShift; idx<data.logset_.size(); idx++)
	lognms.add( data.logset_.getLog(idx).name() );

    logsfld_ = new uiCheckList( loggrp, uiCheckList::Unrel, OD::Horizontal );
    logsfld_->addItems( lognms );
    logsfld_->attach( rightOf, logchk_ );

    saveasfld_ = new uiGenInput( loggrp, uiStrings::sSaveAs(),
			BoolInpSpec(true,tr("Log"),tr("Seismic cube")) );
    saveasfld_->attach( alignedBelow, logsfld_ );
    saveasfld_->valuechanged.notify(
			mCB(this,uiSaveDataDlg,changeLogUIOutput) );

    outputgrp_ = new uiCreateLogCubeOutputSel( loggrp, true );
    outputgrp_->attach( leftAlignedBelow, saveasfld_ );
    changeLogUIOutput(0);

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, loggrp );

    uiGroup* wvltgrp = new uiGroup( this, "Wavelet parameters" );
    wvltgrp->attach( ensureBelow, horSepar );

    uiString wtxt( uiStrings::sWavelet() ); wtxt.append( ":" );
    uiLabel* wvltlbl = new uiLabel( wvltgrp, wtxt );
    wvltlbl->attach( leftBorder );

    uiString txt = tr("Sample interval %1").arg(SI().getUiZUnitString());
    samplefld_ = new uiGenInput( wvltgrp, txt, FloatInpSpec() );
    samplefld_->setValue(
	data.estimatedwvlt_.sampleRate() * SI().zDomain().userFactor() );
    samplefld_->setElemSzPol( uiObject::Small );
    samplefld_->attach( ensureBelow, wvltlbl );

    IOObjContext ctxt = mIOObjContext(Wavelet);
    ctxt.forread_ = false;
    uiIOObjSel::Setup su( tr("Initial wavelet") ); su.optional( true );

    initwvltsel_ = new uiIOObjSel( wvltgrp, ctxt, su );
    initwvltsel_->setInputText( data.initwvlt_.name() );
    initwvltsel_->attach( alignedBelow, samplefld_ );

    su.seltxt_ = tr("Deterministic wavelet");
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


#define mAppMsg(locmsg,act) { msg.append( locmsg, msg.isSet() ); act; }


bool uiSaveDataDlg::saveLogs()
{
    const Data& data = dataserver_.data();
    const bool savetolog = saveasfld_->getBoolValue();

    Well::LogSet logset;
    BufferStringSet lognms;
    uiString msg;
    for ( int ilog=0; ilog<logsfld_->size(); ilog++ )
    {
	if ( !logsfld_->isChecked(ilog) )
	    continue;

	const Well::Log& log = data.logset_.getLog( ilog+cLogShift );
	BufferString lognm( log.name() );
	if ( savetolog )
	    lognm.addSpace().add( outputgrp_->getPostFix() );

	if ( data.wd_->logs().getLog(lognm.buf()) )
	{
	    const uiString localmsg = tr( "Log: '%1' already exists" )
					  .arg( lognm );
	    mAppMsg( localmsg, continue )
	}

	Well::Log* newlog = new Well::Log( log );
	newlog->setName( lognm );
	logset.add( newlog );
	lognms.add( lognm );
    }

    uiString endmsg = tr( "Please choose another postfix" );
    if ( msg.isSet() )
	mAppMsg( endmsg, mErrRet( msg ) )

    DataWriter& datawtr = dataserver_.dataWriter();
    endmsg = tr( "Check your permissions" );
    if ( !datawtr.writeLogs(logset,savetolog) )
    {
	msg = tr( "Cannot write log(s)", 0, logset.size() );
	mAppMsg( endmsg, mErrRet( msg ) )
    }

    if ( !savetolog )
    {
	const int nrtraces = outputgrp_->getNrRepeatTrcs();
	Well::ExtractParams wep;
	wep.setFixedRange( data.getModelRange(), true );
	LogCubeCreator lcr(lognms, logset, dataserver_.wellID(), wep, nrtraces);
	if ( !lcr.setOutputNm(outputgrp_->getPostFix(),
			      outputgrp_->withWellName()) )
	{
	    if ( !outputgrp_->askOverwrite(lcr.errMsg()) )
		return false;
	    else
		lcr.resetMsg();
	}

	uiTaskRunner* taskrunner = new uiTaskRunner( this );
	if ( !TaskRunner::execute(taskrunner,lcr) || !lcr.isOK() )
	    mErrRet( lcr.errMsg() )

    }

    return true;
}


bool uiSaveDataDlg::saveWvlt( bool isestimated )
{
    uiIOObjSel& wvltsel = isestimated ? *estimatedwvltsel_ : *initwvltsel_;
    if ( !wvltsel.isChecked() )
	return true;

    const Data& data = dataserver_.data();
    Wavelet wvlt( isestimated ? data.estimatedwvlt_ : data.initwvlt_ );
    if ( !wvlt.size() && isestimated )
    {
	uiString msg = tr( "No deterministic wavelet yet" );
	msg.append(
	   tr( "Press 'Quality Control' before saving" ), true );
	mErrRet( msg )
    }

    const IOObj* wvltioobj = wvltsel.ioobj();
    if ( !wvltioobj )
	return false;

    float sr = samplefld_->getFValue();
    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( tr("The sample interval is not valid") )

    sr /= SI().zDomain().userFactor();
    if ( !mIsEqual(sr,wvlt.sampleRate(),mDefEps) )
	wvlt.reSample( sr );

    if ( !wvlt.put(wvltioobj) )
	mErrRet( tr( "Cannot write output wavelet" ) )

    return true;
}


bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    bool success = true;

    if ( logsfld_->firstChecked() == -1 && !initwvltsel_->isChecked() &&
	 !estimatedwvltsel_->isChecked() )
	mErrRet( tr( "Please check at least one item to be saved" ) )

    if ( !saveLogs() || !saveWvlt(false) || !saveWvlt(true) )
	success = false;

    if ( success )
	uiMSG().message( tr( "Successfully saved the selected items" ) );

    return false;
}

} // namespace WellTie
