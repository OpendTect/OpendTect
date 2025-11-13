/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogtools.h"

#include "arrayndimpl.h"
#include "dataclipper.h"
#include "envvars.h"
#include "fftfilter.h"
#include "od_helpids.h"
#include "statgrubbs.h"
#include "smoother1d.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellselection.h"
#include "welltrack.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifreqfilter.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uiwelldisplayserver.h"
#include "uiwelllogtoolsgrp.h"

// WellLogToolData
WellLogToolData::WellLogToolData( const Well::SelInfo& info )
    : Well::SubSelData(info)
{
}


WellLogToolData::~WellLogToolData()
{
    deepErase( outplogs_ );
}


int WellLogToolData::nrLogs() const
{
    return lognms().size();
}


const Well::Log* WellLogToolData::getInpLog( const char* lognm ) const
{
    return wd_ ? wd_->logs().getLog( lognm ) : nullptr;
}


const Well::LogSet* WellLogToolData::getLogs() const
{
    return wd_ ? &wd_->logs() : nullptr;
}


const Well::Log* WellLogToolData::getOutpLog( const char* lognm ) const
{
    return getNonConst(this)->getOutpLog( lognm );
}


Well::Log* WellLogToolData::getOutpLog( const char* lognm )
{
    for ( auto* log : outplogs_ )
	if ( log->name() == lognm )
	    return log;

    return nullptr;
}


bool WellLogToolData::loadInputLogs( uiString& errmsg )
{
    const MultiID wid = wellID();
    const Well::LoadReqs lreqs( lognms() );
    const bool res = Well::MGR().get( wid, lreqs );
    if ( !res )
	errmsg = Well::MGR().errMsg();

    return true;
}


Well::Log* WellLogToolData::addOutputLog( const Well::Log& inplog )
{
    auto* outputlog = new Well::Log( inplog );
    outplogs_.add( outputlog );
    return outputlog;
}


void WellLogToolData::replace( Well::Log* oldlog, Well::Log* newlog )
{
    outplogs_ -= oldlog;
    delete oldlog;
    outplogs_.add( newlog );
}



// uiWellLogToolWinMgr
static const int cPrefWidth = 150;

uiWellLogToolWinMgr::uiWellLogToolWinMgr( uiParent* p,
					  const BufferStringSet* welllnms,
					  const BufferStringSet* lognms )
	: uiDialog(p,Setup(tr("Select Well(s) and Log(s) for Editing"),
			   mODHelpKey(mWellLogToolWinMgrHelpID)))
{
    setOkText( uiStrings::sContinue() );
    uiWellExtractParams::Setup su;
    su.withzintime_ = su.withextractintime_ = false;
    welllogselfld_ = new uiMultiWellLogSel( this, su, welllnms, lognms );
    welllogselfld_->selectOnlyWritableWells();
}


uiWellLogToolWinMgr::~uiWellLogToolWinMgr()
{}


#define mErrRet(s) { uiMSG().error(s); return false; }

int uiWellLogToolWinMgr::checkMaxLogsToDisplay()
{
    const bool limitbyscreensz = GetEnvVarYN( "OD_MAX_LOGS_SCREEN", false );
    if ( limitbyscreensz )
    {
	uiMain& uimain = uiMain::instance();
	const uiSize sz( uimain.getScreenSize(0,true) );
	return sz.width()/cPrefWidth;
    }

    return 999;
}


bool uiWellLogToolWinMgr::acceptOK( CallBacker* )
{
    TypeSet<MultiID> wellids;
    BufferStringSet  wellnms, lognms;
    welllogselfld_->getSelWellIDs( wellids );
    welllogselfld_->getSelWellNames( wellnms );
    welllogselfld_->getSelLogNames( lognms );
    if ( wellids.isEmpty() )
	mErrRet( tr("Please select at least one well") )

    ObjectSet<WellLogToolData> logdatas;
    uiRetVal uirv;
    int totalnrlogs = 0;
    const int maxlimit = checkMaxLogsToDisplay();
    const Well::ExtractParams& params = welllogselfld_->params();
    Well::LoadReqs lreqs( false );
    params.fill( lreqs );
    lreqs.include( Well::LogInfos );
    for ( const auto& wid : wellids )
    {
	RefMan<Well::Data> wd = Well::MGR().get( wid, lreqs );
	if ( !wd )
	{
	    uirv.add( Well::MGR().errMsg() );
	    continue;
	}

	const Well::LogSet& logs = wd->logs();
	BufferStringSet wlllognms;
	for ( const auto* lognm : lognms )
	{
	    if ( logs.isPresent(lognm->buf()) )
		wlllognms.add( lognm->buf() );
	}

	if ( wlllognms.isEmpty() )
	    continue;

	Well::SelInfo info( *wd );
	info.setSelectedLogs( wlllognms );
	uiString errmsg;
	const Interval<float> dahrg = params.calcFrom( *wd, wlllognms, errmsg );
	if ( dahrg.isUdf() )
	{
	    uirv.add( errmsg );
	    continue;
	}

	info.setMDRange( dahrg );
	PtrMan<WellLogToolData> ldata = new WellLogToolData( info );
	const int nrinplogs = ldata->nrLogs();
	if ( nrinplogs < 1 )
	    continue;

	totalnrlogs += nrinplogs;
	if ( totalnrlogs > maxlimit )
	    break;

	logdatas += ldata.release();
    }

    if ( logdatas.isEmpty() )
    {
	uiMSG().errorWithDetails( uirv,
				    tr("Please select at least one valid "
				       "log for the selected well(s)") );
	return false;
    }
    else if ( logdatas.size() < wellids.size() )
    {
	uiMSG().messageWithDetails( uirv,
		tr("Could not extract data for all selected wells") );
    }

    if ( totalnrlogs > maxlimit )
    {
	uiString msg = tr("You have selected %1 logs. Unfortunately OpendTect\n"
			  "can only display %2 logs on this screen.\n"
			  "Do you want to display logs of the first")
			.arg( totalnrlogs ).arg( maxlimit );

	const int ldsize = logdatas.size();
	ldsize == 1 ? msg.append( tr("well?") )
		    : msg.append( tr("%3 wells?").arg(ldsize) );

	if ( !uiMSG().askGoOn(msg) )
	{
	    deepErase( logdatas );
	    return false;
	}
    }

    uirv.setOK();
    for ( auto* logdata : logdatas )
    {
	uiString errmsg;
	if ( !logdata->loadInputLogs(errmsg) )
	    uirv.add( errmsg );
    }

    if ( uirv.isError() )
    {
	uiMSG().messageWithDetails( uirv,
				    tr("Could not load all selected logs") );
    }

    auto* win = new uiWellLogToolWin( this, logdatas );
    win->show();
    win->windowClosed.notify( mCB(this,uiWellLogToolWinMgr,winClosed) );

    return false;
}


void uiWellLogToolWinMgr::winClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellLogToolWin*,win,cb)
    if ( !win )
    {
	pErrMsg( "cb null or not uiWellLogToolWin" );
	return;
    }

    welllogselfld_->update();
}


// uiWellLogToolWin
uiWellLogToolWin::uiWellLogToolWin( uiParent* p,
				    ObjectSet<WellLogToolData>& logs,
				    bool withedit )
    : uiMainWin(p,Setup(tr("Log Tools Window")).nrstatusflds(0))
    , logdatas_(logs)
{
    logdisp_ = GetWellDisplayServer().createWellLogToolGrp( this, logdatas_ );

    uiGroup* editgrp = withedit ? createEditGroup() : nullptr;
    if ( editgrp )
	editgrp->attach( ensureBelow, logdisp_ );

    auto* horSepar = new uiSeparator( this );
    if ( editgrp )
	horSepar->attach( stretchedBelow, editgrp->attachObj() );
    else
	horSepar->attach( stretchedBelow, logdisp_ );

    okbut_ = uiButton::getStd( this, OD::Ok,
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut_->attach( leftBorder, 20 );
    okbut_->attach( ensureBelow, horSepar );
    okbut_->setSensitive( false );

    auto* cancelbut = uiButton::getStd( this, OD::Cancel,
				mCB(this,uiWellLogToolWin,rejectOK), true );
    cancelbut->attach( rightBorder, 20 );
    cancelbut->attach( ensureBelow, horSepar );

    logdisp_->displayLogs();
}


uiGroup* uiWellLogToolWin::createEditGroup()
{
    auto* editgrp = new uiGroup( this, "Edit" );
    auto* actiongrp = new uiGroup( editgrp, "Action" );
    actiongrp->attach( hCentered );
    const char* acts[] =
	{ "Remove Spikes", "FFT Filter", "Smooth",
	  "Clip", "Upscale", "Resample", "Remove isolated undefined", nullptr };
    auto* llc = new uiLabeledComboBox( actiongrp, acts, uiStrings::sAction() );
    actionfld_ = llc->box();
    actionfld_->selectionChanged.notify(mCB(this,uiWellLogToolWin,actionSelCB));

    CallBack cb( mCB( this, uiWellLogToolWin, applyPushedCB ) );
    applybut_ = uiButton::getStd( actiongrp, OD::Apply, cb, true );
    applybut_->attach( rightOf, llc );

    freqfld_ = new uiFreqFilter( actiongrp );
    freqfld_->attach( alignedBelow, llc );

    auto* spbgt = new uiLabeledSpinBox( actiongrp,tr("Window size (samples)") );
    spbgt->attach( alignedBelow, llc );
    gatefld_ = spbgt->box();
    gatelbl_ = spbgt->label();

    const uiString txt = tr("Threshold (Grubbs number)");
    thresholdfld_ = new uiLabeledSpinBox( actiongrp, txt );
    thresholdfld_->attach( rightOf, spbgt );
    thresholdfld_->box()->setInterval( 1.0, 20.0, 0.1 );
    thresholdfld_->box()->setValue( 3 );
    thresholdfld_->box()->setNrDecimals( 2 );

    const char* spk[] =
	{"Undefined values","Interpolated values","Specify", nullptr };
    replacespikefld_ =
	new uiLabeledComboBox( actiongrp, spk, tr("Replace spikes by") );
    replacespikefld_->box()->selectionChanged.notify(
			mCB(this,uiWellLogToolWin,handleSpikeSelCB) );
    replacespikefld_->attach( alignedBelow, spbgt );

    replacespikevalfld_ = new uiGenInput( actiongrp, uiStrings::sEmptyString(),
					  FloatInpSpec() );
    replacespikevalfld_->attach( rightOf, replacespikefld_ );
    replacespikevalfld_->setValue( 0 );

    auto* savegrp = new uiGroup( editgrp, "Save options" );
    savegrp->attach( alignedBelow, actiongrp );
    savefld_ = new uiGenInput( savegrp, tr("On OK"),
	BoolInpSpec(true,tr("Save logs as new"),tr("Overwrite original logs")));
    savefld_->valueChanged.notify( mCB(this,uiWellLogToolWin,saveCB) );

    extfld_ = new uiGenInput( savegrp, tr("Log name extension") );
    extfld_->setText( "_edited" );
    extfld_->attach( alignedBelow, savefld_ );

    actionSelCB(nullptr);
    saveCB(nullptr);
    return editgrp;
}


uiWellLogToolWin::~uiWellLogToolWin()
{
    deepErase( logdatas_ );
}


void uiWellLogToolWin::saveCB( CallBacker* )
{
    extfld_->display( savefld_->getBoolValue() );
}


void uiWellLogToolWin::actionSelCB( CallBacker* )
{
    const int act = actionfld_->currentItem();
    extfld_->setText( "_edited" );
    thresholdfld_->display( act == 0 );
    replacespikevalfld_->display( act == 0 );
    replacespikefld_->display( act == 0 );
    freqfld_->display( act == 1 );
    gatefld_->display( act != 1 );
    gatelbl_->display( act != 1 );
    if ( act == 0 )
    {
	gatelbl_->setText( tr("Window size (samples)") );
	gatefld_->setNrDecimals( 0 );
	gatefld_->setInterval( StepInterval<int>(1,1500,5) );
	gatefld_->setValue( 300 );
    }
    else if ( act == 1 )
    {}
    else if ( act == 2 )
    {
	gatelbl_->setText( tr("Window size (samples)") );
	gatefld_->setNrDecimals( 0 );
	gatefld_->setInterval( StepInterval<int>(1,1500,5) );
	gatefld_->setValue( 300 );
    }
    else if ( act == 3 )
    {
	gatelbl_->setText( tr("Clip rate (%)") );
	gatefld_->setNrDecimals( 0 );
	gatefld_->setInterval( StepInterval<int>(0,100,10) );
	gatefld_->setValue( 1 );
    }
    else if ( act == 4 )
    {
	const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
	gatelbl_->setText( tr("Sample interval %1").
		    arg(UnitOfMeasure::surveyDefDepthUnitAnnot( true, true )) );
	gatefld_->setNrDecimals( 4 );
	gatefld_->setInterval( StepInterval<float>(0.1,10,0.1) );
	gatefld_->setValue( uom->isImperial() ? 0.5 : 0.1524 );
	extfld_->setText( "_upscaled" );
    }
    else if ( act == 5 )
    {
	const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
	gatelbl_->setText( tr("Sample interval %1").
		    arg(UnitOfMeasure::surveyDefDepthUnitAnnot( true, true )) );
	gatefld_->setNrDecimals( 4 );
	gatefld_->setInterval( StepInterval<float>(0.1,10,0.1) );
	gatefld_->setValue( uom->isImperial() ? 0.5 : 0.1524 );
	extfld_->setText( "_resampled" );
    }
    else if ( act == 6 )
    {
	gatefld_->display( false );
	gatelbl_->display( false );
	extfld_->setText( "_udfcleaned" );
    }

    handleSpikeSelCB(0);
}


void uiWellLogToolWin::handleSpikeSelCB( CallBacker* )
{
    const int act = replacespikefld_->box()->currentItem();
    replacespikevalfld_->display( act == 2 );
}


void uiWellLogToolWin::acceptOK( CallBacker* )
{
    closeok_ = savefld_ ? saveLogs() : true;
    close();
}


bool uiWellLogToolWin::closeOK()
{
    const bool closeok = closeok_;
    closeok_ = true;
    return closeok;
}


void uiWellLogToolWin::rejectOK( CallBacker* )
{
    closeok_ = true;
    close();
}


#define mAddErrMsg( msg ) \
{ \
    if ( emsg.isEmpty() ) \
	emsg.set( msg ); \
    else \
	emsg.appendPhrase( msg, uiString::NoSep ); \
    continue; \
}

void uiWellLogToolWin::applyPushedCB( CallBacker* )
{
    const int act = actionfld_->currentItem();
    uiString emsg;
    for ( int idldata=0; idldata<logdatas_.size(); idldata++ )
    {
	WellLogToolData& ld = *logdatas_[idldata];
	if ( !ld.isOK() )
	    continue;

	const char* wllnm = ld.wellName();
	const BufferStringSet& lognms = ld.lognms();
	for ( const auto* lognm : lognms )
	{
	    const Well::Log* inplog = ld.getInpLog( lognm->buf() );
	    if ( !inplog || !inplog->isLoaded() )
		continue;

	    const int sz = inplog->size();
	    if ( sz<2 )
		continue;

	    auto* outplog = ld.getOutpLog( lognm->buf() );
	    if ( outplog )
		*outplog = *inplog;
	    else
		outplog = ld.addOutputLog( *inplog );

	    if ( act == 0 || act == 2 )
	    {
		const int gatesz = gatefld_->getIntValue();
		if ( sz < 2*gatesz )
		    continue;
	    }

	    if ( act == 0 )
	    {
		Stats::Grubbs sgb;
		const float cutoff_grups = thresholdfld_->box()->getFValue();
		TypeSet<int> grubbsidxs;
		const int winsz = gatefld_->getIntValue();
		mAllocVarLenArr( float, gatevals, winsz )
		float* outpvals = outplog->valArr();
		for ( int idx=winsz/2; idx<sz-winsz; idx+=winsz  )
		{
		    float cutoffval = cutoff_grups + 1;
		    while ( cutoffval > cutoff_grups )
		    {
			for (int winidx=0; winidx<winsz; winidx++)
			    gatevals[winidx]= outpvals[idx+winidx-winsz/2];

			int idxtofix;
			cutoffval = sgb.getMax( mVarLenArr(gatevals),
						winsz, idxtofix ) ;
			if ( cutoffval > cutoff_grups  && idxtofix >= 0 )
			{
			    outpvals[idx+idxtofix-winsz/2] = mUdf( float );
			    grubbsidxs += idx+idxtofix-winsz/2;
			}
		    }
		}
		const int spkact = replacespikefld_->box()->currentItem();
		for ( int idx=0; idx<grubbsidxs.size(); idx++ )
		{
		    const int gridx = grubbsidxs[idx];
		    float& grval = outpvals[gridx];
		    if ( spkact == 2 )
		    {
			grval = replacespikevalfld_->getFValue();
		    }
		    else if ( spkact == 1 )
		    {
			float dah = outplog->dah( gridx );
			grval = outplog->getValue( dah, true );
		    }
		}
	    }
	    else if ( act == 1 )
	    {
		RefMan<Well::Data> wd = Well::MGR().get( ld.wellID(),
				Well::LoadReqs(Well::Trck,Well::D2T) );
		const Well::Track& track = wd->track();
		const float startdah = outplog->dahRange().start_;
		const float stopdah = outplog->dahRange().stop_;
		const float zstart = sCast( float,
					    track.getPos( startdah ).z_ );
                const float zstop = sCast( float, track.getPos( stopdah ).z_ );
		const Interval<float> zrg( zstart, zstop );
		ObjectSet<const Well::Log> reslogs;
		reslogs += outplog;
		Stats::UpscaleType ut = Stats::UseAvg;
		const float deftimestep = 0.001f;

		const Well::D2TModel* d2tm = wd->d2TModel();
		Well::LogSampler ls( d2tm, &track, zrg, false, deftimestep,
				     SI().zIsTime(), ut, reslogs );
		if ( !ls.execute() )
		    mAddErrMsg(tr("Could not resample the logs at well %1").
								    arg(wllnm))

		const int size = ls.nrZSamples();
		Array1DImpl<float> logvals( size );
		for ( int idz=0; idz<size; idz++ )
		    logvals.set( idz, ls.getLogVal( 0, idz ) );

		const TypeSet<float> freqrg = freqfld_->frequencies();
		FFTFilter filter( size, deftimestep );
		if ( freqfld_->filterType() == FFTFilter::HighPass )
		    filter.setHighPass( freqrg[0], freqrg[1] );
		else if ( freqfld_->filterType() == FFTFilter::LowPass )
		    filter.setLowPass( freqrg[0], freqrg[1] );
		else
		    filter.setBandPass( freqrg[0], freqrg[1],
					freqrg[2], freqrg[3] );

		if ( !filter.apply(logvals) )
		    mAddErrMsg(tr("Could not apply the FFT Filter at well %1").
								    arg(wllnm))

		PointBasedMathFunction filtvals( OD::InterpolationType::Linear,
					OD::ExtrapolationType::EndValue );
		for ( int idz=0; idz<size; idz++ )
		{
		    const float val = logvals.get( idz );
		    if ( mIsUdf(val) )
			continue;

		    filtvals.add( ls.getDah( idz ), logvals.get( idz ) );
		}

		float* outpvals = outplog->valArr();
		for ( int idz=0; idz<sz; idz++ )
		{
		    const float dah = outplog->dah( idz );
		    outpvals[idz] = filtvals.getValue( dah );
		}

		if ( freqfld_->filterType() != FFTFilter::LowPass )
		    outplog->setMnemonicLabel( nullptr );
	    }
	    else if ( act == 2 )
	    {
		const float* inpvals = inplog->valArr();
		float* outpvals = outplog->valArr();
		Smoother1D<float> sm;
		sm.setInput( inpvals, sz );
		sm.setOutput( outpvals );
		const int winsz = gatefld_->getIntValue();
		sm.setWindow( HanningWindow::sName(), 0.95, winsz );
		if ( !sm.execute() )
		    mAddErrMsg(
			tr("Could not apply the smoothing window at well %1").
								    arg(wllnm))
	    }
	    else if ( act == 3 )
	    {
		float* outpvals = outplog->valArr();
		Interval<float> rg;
		const float rate = gatefld_->getFValue() / 100.f;
		DataClipSampler dcs( sz );
		dcs.add( outpvals, sz );
		rg = dcs.getRange( rate );
		for ( int idx=0; idx<sz; idx++ )
		{
		    if ( outpvals[idx] < rg.start_ ) outpvals[idx] = rg.start_;
		    if ( outpvals[idx] > rg.stop_ )  outpvals[idx] = rg.stop_;
		}
	    }
	    else if ( act == 4 )
	    {
		StepInterval<float> rg( inplog->dahRange() );
		rg.step_ = gatefld_->getFValue();
		Well::Log* upscaledlog = inplog->upScaleLog( rg );
		ld.replace( outplog, upscaledlog );
	    }
	    else if ( act == 5 )
	    {
		StepInterval<float> rg( inplog->dahRange() );
		rg.step_ = gatefld_->getFValue();
		Well::Log* sampledlog = inplog->sampleLog( rg );
		ld.replace( outplog, sampledlog );
	    }
	    else if ( act == 6 )
	    {
		Well::Log* cleanudflog = inplog->cleanUdfs();
		ld.replace( outplog, cleanudflog );
	    }

	    outplog->updateAfterValueChanges();
	}
    }

    okbut_->setSensitive( emsg.isEmpty() );
    if ( !emsg.isEmpty() )
	uiMSG().error( emsg );

    logdisp_->displayLogs();
}


bool uiWellLogToolWin::saveLogs()
{
    const bool overwrite = !savefld_->getBoolValue();
    if ( overwrite )
    {
	const bool res = uiMSG().askOverwrite( tr("Are you sure you want "
		"to overwrite the original logs?") );
	if ( !res )
	    return false;
    }

    uiRetVal uirv;
    for ( auto* logdata : logdatas_ )
    {
	const Well::LogSet* logs = logdata->getLogs();
	if ( !logs )
	    continue;

	ObjectSet<Well::Log>& outputlogs = logdata->outpLogs();
	for ( auto* log : outputlogs )
	{
	    BufferString newnm( log->name() );
	    newnm += extfld_->text();
	    if ( !overwrite && logs->isPresent(newnm.buf()) )
	    {
		uiMSG().error(
		    tr("One or more logs with this name already exists."
		    "\nPlease select a different extension for the new logs"));
		return false;
	    }

	    if ( !overwrite )
		log->setName( newnm.buf() );
	}

	const bool res = Well::MGR().writeAndRegister( logdata->wellID(),
						       outputlogs );
	if ( !res )
	    uirv.add( Well::MGR().errMsg() );
    }

    if ( uirv.isError() )
    {
	uiMSG().errorWithDetails( uirv, tr("Error saving edited logs") );
	return false;
    }

    return true;
}


// uiWellLogEditor
uiWellLogEditor::uiWellLogEditor( uiParent* p, Well::Log& log )
    : uiDialog(p,Setup(tr("Edit Well log"),mODHelpKey(mWellLogEditorHelpID)))
    , valueChanged(this)
    , log_(log)
{
    uiString dlgcaption = uiStrings::phrEdit(uiStrings::phrJoinStrings(
				     toUiString("'%1'").arg(toUiString(
				     log.name())),uiStrings::sLog().toLower()));
    setCaption( dlgcaption );
    uiTable::Setup ts( log_.size(), 2 ); ts.rowgrow(true);
    table_ = new uiTable( this, ts, "Well log table" );
    table_->setSelectionMode( uiTable::Multi );
    table_->setSelectionBehavior( uiTable::SelectRows );
    mAttachCB( table_->rowDeleted, uiWellLogEditor::rowDelCB );
    mAttachCB( table_->selectionDeleted, uiWellLogEditor::rowDelCB );
    mAttachCB( table_->rowInserted, uiWellLogEditor::rowInsertCB );

    const bool depthsinfeet = SI().depthsInFeet();
    const uiString depthunitstr =
		uiStrings::sDistUnitString( depthsinfeet, true, false );
    uiString mdlbl = toUiString("MD (%1)").arg( depthunitstr );
    uiString loglbl = toUiString(log_.name());
    if ( log_.haveUnit() )
	loglbl = toUiString("%1 (%2)").arg(loglbl).arg( log.unitMeasLabel() );

    uiStringSet colnms; colnms.add(mdlbl).add(loglbl);
    table_->setColumnLabels( colnms );

    fillTable();
    mAttachCB( table_->valueChanged, uiWellLogEditor::valChgCB );
}


uiWellLogEditor::~uiWellLogEditor()
{
    detachAllNotifiers();
}


void uiWellLogEditor::fillTable()
{
    NotifyStopper ns( table_->valueChanged );
    const int sz = log_.size();
    const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float md = uom ? uom->userValue( log_.dah(idx) ) : log_.dah(idx);
	table_->setValue( RowCol(idx,0), md, 0, 'f', 4 );
	table_->setValue( RowCol(idx,1), log_.value(idx), 0, 'g', 8 );
	//TODO: use per-mnemonic format and precision?
    }
}


void uiWellLogEditor::selectMD( float md )
{
    const int mdidx = log_.indexOf( md );
    table_->selectRow( mdidx );
}


void uiWellLogEditor::valChgCB( CallBacker* )
{
    const RowCol& rc = table_->notifiedCell();

    if ( rc.row()<0 || rc.row()>=log_.size() )
	return;

    const bool mdchanged = rc.col() == 0;
    const float newval = table_->getFValue( rc );
    const float oldval = mdchanged ? log_.dah( rc.row() )
				   : log_.value( rc.row() );
    if ( mIsEqual(oldval,newval,mDefEpsF) )
	return;

    if ( mdchanged )
    {
	float prevmdval = 0.f;
	float nextmdval = 0.f;
	bool ismdok = false;

	if ( rc.row() != 0 )
	{
	    prevmdval = log_.dah( rc.row()-1 );
	    ismdok = newval > prevmdval;
	    if ( !ismdok )
	    {
		uiMSG().error(tr("The MD value entered is less than the "
				 "previous MD value. Please Change."));
		return;
	    }

	}

	if ( rc.row() < log_.size()-1 )
	{
	    nextmdval = log_.dah( rc.row()+1 );
	    ismdok = newval < nextmdval;
	    if ( !ismdok )
	    {
		uiMSG().error(tr("The MD value entered is greater than the "
				 "next MD value. Please Change."));
		return;
	    }
	}

	if ( ismdok )
	    log_.dahArr()[rc.row()] = newval;
    }
    else
	log_.setValue( rc.row(), newval );

    changed_ = true;
    valueChanged.trigger();
}


void uiWellLogEditor::rowDelCB( CallBacker* )
{
    const TypeSet<int> rowidxset = table_->getNotifRCs();
    for( int idx=rowidxset.size()-1; idx>=0; idx-- )
    {
	int rowidx = rowidxset[idx];
	log_.remove( rowidx );
    }
    changed_ = true;
    valueChanged.trigger();
}


void uiWellLogEditor::rowInsertCB( CallBacker* )
{
    table_->setDefaultRowLabels();
    const int rownr = table_->currentRow();
    float prevmdval = 0.f;
    float nextmdval = 0.f;

    if ( rownr != 0 )
	prevmdval = log_.dah( rownr-1 );

    if ( rownr < log_.size()-1 )
	nextmdval = log_.dah( rownr );

    log_.insertAtDah( (prevmdval+nextmdval)/2, 0.f );
    valueChanged.trigger();
}


bool uiWellLogEditor::acceptOK( CallBacker* )
{
    return true;
}
