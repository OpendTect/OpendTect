/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogtools.h"

#include "arrayndimpl.h"
#include "color.h"
#include "dataclipper.h"
#include "envvars.h"
#include "fftfilter.h"
#include "fourier.h"
#include "od_helpids.h"
#include "statgrubbs.h"
#include "smoother1d.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellselection.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellwriter.h"

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
#include "uitaskrunner.h"
#include "uiwelldisplayserver.h"
#include "uiwelllogtoolsgrp.h"

#include "hiddenparam.h"

HiddenParam<WellLogToolData, ObjectSet<Well::Log>*> hp_outlogs_(nullptr);

// WellLogToolData
WellLogToolData::WellLogToolData( const Well::SelInfo& info )
    : Well::SubSelData(info)
{
    hp_outlogs_.setParam( this, new ObjectSet<Well::Log> );
    init();
}


WellLogToolData::~WellLogToolData()
{
    inplogs_.setEmpty();
    deepErase( outpLogs() );
    hp_outlogs_.removeAndDeleteParam( this );
}


ObjectSet<Well::Log>& WellLogToolData::outpLogs()
{
    return *hp_outlogs_.getParam( this );
}


const ObjectSet<Well::Log>& WellLogToolData::outpLogs() const
{
    return *hp_outlogs_.getParam( this );
}


void WellLogToolData::init()
{
    for ( int idx=0; idx<logs().size(); idx++ )
    {
	const Well::Log& log = logs().getLog( idx );
	inplogs_ += &log;

	auto* outplog = new Well::Log( log );
	outpLogs().add( outplog );
    }
}



// uiWellLogToolWinMgr
static const int cPrefWidth = 150;

uiWellLogToolWinMgr::uiWellLogToolWinMgr( uiParent* p,
					  const BufferStringSet* welllnms,
					  const BufferStringSet* lognms )
	: uiDialog(p,Setup(tr("Select Well(s) and Log(s) for Editing"),
		     mNoDlgTitle,mODHelpKey(mWellLogToolWinMgrHelpID)))
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
    BufferStringSet msgs;
    int nrsellogs = lognms.size() * wellnms.size();
    int totalnrlogs = 0;
    const int maxlimit = checkMaxLogsToDisplay();
    bool displogs = false;
    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const MultiID& wmid = wellids[idx];
	const Well::LoadReqs req( Well::LogInfos );
	RefMan<Well::Data> wd = Well::MGR().get( wmid, req );
	Well::SelInfo info( *wd );
	info.setSelectedLogs( lognms );
	const Well::ExtractParams& params = welllogselfld_->params();
	info.setMDRange( params.calcFrom(*wd,lognms,true) );
	auto* ldata = new WellLogToolData( info );
	const int nrinplogs = ldata->inpLogs().size();
	if ( !nrinplogs )
	{
	    delete ldata;
	    continue;
	}

	totalnrlogs += nrinplogs;
	displogs = totalnrlogs <= maxlimit;
	if ( !displogs )
	{
	    delete ldata;
	    break;
	}

	logdatas += ldata;
    }

    if ( !displogs )
    {
	const int ldsize = logdatas.size();
	uiString msg = tr("You have selected %1 logs. Unfortunately OpendTect\n"
			  "can only display %2 logs on this screen.\n"
			  "Do you want to display logs of the first")
			.arg(nrsellogs).arg(maxlimit);

	ldsize == 1 ? msg.append( tr("well?") )
		    : msg.append( tr("%3 wells?").arg(ldsize));

	const int res = uiMSG().askGoOn( msg );
	if ( !res )
	{
	    deepErase(logdatas);
	    return false;
	}
    }

    if ( logdatas.isEmpty() )
	mErrRet(tr("%1\nPlease select at least one valid "
		   "log for the selected well(s)")
		   .arg(msgs.cat()) )
    else if ( !msgs.isEmpty() )
	uiMSG().warning( tr("%1\nWill process the other wells only")
			     .arg( msgs.cat() ) );


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
    , actionfld_(nullptr)
    , savefld_(nullptr)
    , logdatas_(logs)
{
    logdisp_ = GetWellDisplayServer().createWellLogToolGrp( this, logdatas_ );

    uiGroup* editgrp = withedit ? createEditGroup() : nullptr;
    if ( editgrp )
	editgrp->attach( ensureBelow, logdisp_ );

    uiSeparator* horSepar = new uiSeparator( this );
    if ( editgrp )
	horSepar->attach( stretchedBelow, editgrp->attachObj() );
    else
	horSepar->attach( stretchedBelow, logdisp_ );


    okbut_ = uiButton::getStd( this, OD::Ok,
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut_->attach( leftBorder, 20 );
    okbut_->attach( ensureBelow, horSepar );
    okbut_->setSensitive( false );

    uiButton* cancelbut = uiButton::getStd( this, OD::Cancel,
				mCB(this,uiWellLogToolWin,rejectOK), true );
    cancelbut->attach( rightBorder, 20 );
    cancelbut->attach( ensureBelow, horSepar );

    logdisp_->displayLogs();
}


uiGroup* uiWellLogToolWin::createEditGroup()
{
    uiGroup* editgrp = new uiGroup( this, "Edit" );
    uiGroup* actiongrp = new uiGroup( editgrp, "Action" );
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

    freqfld_ = new uiFreqFilterSelFreq( actiongrp );
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

    uiGroup* savegrp = new uiGroup( editgrp, "Save options" );
    savegrp->attach( alignedBelow, actiongrp );
    savefld_ = new uiGenInput( savegrp, tr("On OK"),
	BoolInpSpec(true,tr("Save logs as new"),tr("Overwrite original logs")));
    savefld_->valuechanged.notify( mCB(this,uiWellLogToolWin,saveCB) );

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


#define mAddErrMsg( msg, well ) \
{ \
    if ( emsg.isEmpty() ) \
	emsg.set( msg ); \
    else \
	emsg.add( "\n" ).add( msg ); \
	emsg.add( " at well " ).add( well ); \
    continue; \
}

void uiWellLogToolWin::applyPushedCB( CallBacker* )
{
    const int act = actionfld_->currentItem();
    BufferString emsg;
    for ( int idldata=0; idldata<logdatas_.size(); idldata++ )
    {
	WellLogToolData& ld = *logdatas_[idldata];
	const char* wllnm = ld.wellName();

	for ( int idlog=0; idlog<ld.inpLogs().size(); idlog++ )
	{
	    const Well::Log& inplog = *ld.inpLogs().get( idlog );
	    Well::Log& outplog = *ld.outpLogs().get( idlog );
	    const int sz = inplog.size();
	    if ( sz< 2 )
		continue;

	    if ( act == 0 || act == 2 )
	    {
		const int gatesz = gatefld_->getIntValue();
		if ( sz < 2*gatesz )
		    continue;
	    }

	    const float* inp = inplog.valArr();
	    float* outp = outplog.valArr();
	    if ( act == 0 )
	    {
		Stats::Grubbs sgb;
		const float cutoff_grups = thresholdfld_->box()->getFValue();
		TypeSet<int> grubbsidxs;
		const int winsz = gatefld_->getIntValue();
		mAllocVarLenArr( float, gatevals, winsz )
		for ( int idx=winsz/2; idx<sz-winsz; idx+=winsz  )
		{
		    float cutoffval = cutoff_grups + 1;
		    while ( cutoffval > cutoff_grups )
		    {
			for (int winidx=0; winidx<winsz; winidx++)
			    gatevals[winidx]= outp[idx+winidx-winsz/2];

			int idxtofix;
			cutoffval = sgb.getMax( mVarLenArr(gatevals),
						winsz, idxtofix ) ;
			if ( cutoffval > cutoff_grups  && idxtofix >= 0 )
			{
			    outp[idx+idxtofix-winsz/2] = mUdf( float );
			    grubbsidxs += idx+idxtofix-winsz/2;
			}
		    }
		}
		const int spkact = replacespikefld_->box()->currentItem();
		for ( int idx=0; idx<grubbsidxs.size(); idx++ )
		{
		    const int gridx = grubbsidxs[idx];
		    float& grval = outp[gridx];
		    if ( spkact == 2 )
		    {
			grval = replacespikevalfld_->getFValue();
		    }
		    else if ( spkact == 1 )
		    {
			float dah = outplog.dah( gridx );
			grval = outplog.getValue( dah, true );
		    }
		}
	    }
	    else if ( act == 1 )
	    {
		RefMan<Well::Data> wd = Well::MGR().get( ld.wellID(),
				Well::LoadReqs(Well::Trck,Well::D2T) );
		const Well::Track& track = wd->track();
		const float startdah = outplog.dahRange().start;
		const float stopdah = outplog.dahRange().stop;
		const float zstart = sCast( float, track.getPos( startdah ).z );
		const float zstop = sCast( float, track.getPos( stopdah ).z );
		const Interval<float> zrg( zstart, zstop );
		ObjectSet<const Well::Log> reslogs;
		reslogs += &outplog;
		Stats::UpscaleType ut = Stats::UseAvg;
		const float deftimestep = 0.001f;

		const Well::D2TModel* d2tm = wd->d2TModel();
		Well::LogSampler ls( d2tm, &track, zrg, false, deftimestep,
				     SI().zIsTime(), ut, reslogs );
		if ( !ls.execute() )
		    mAddErrMsg( "Could not resample the logs", wllnm )

		const int size = ls.nrZSamples();
		Array1DImpl<float> logvals( size );
		for ( int idz=0; idz<size; idz++ )
		    logvals.set( idz, ls.getLogVal( 0, idz ) );

		const Interval<float>& freqrg = freqfld_->freqRange();
		FFTFilter filter( size, deftimestep );
		if ( freqfld_->filterType() == FFTFilter::HighPass )
		    filter.setHighPass( freqrg.start );
		else if ( freqfld_->filterType() == FFTFilter::LowPass )
		    filter.setLowPass( freqrg.stop );
		else
		{
		    if ( freqrg.isRev() )
			mAddErrMsg( "Taper start frequency must be larger"
				       " than stop frequency", wllnm )

		    filter.setBandPass( freqrg.start, freqrg.stop );
		}

		if ( !filter.apply(logvals) )
		    mAddErrMsg( "Could not apply the FFT Filter", wllnm )

		PointBasedMathFunction filtvals(PointBasedMathFunction::Linear,
						PointBasedMathFunction::EndVal);
		for ( int idz=0; idz<size; idz++ )
		{
		    const float val = logvals.get( idz );
		    if ( mIsUdf(val) )
			continue;

		    filtvals.add( ls.getDah( idz ), logvals.get( idz ) );
		}

		for ( int idz=0; idz<sz; idz++ )
		{
		    const float dah = outplog.dah( idz );
		    outp[idz] = filtvals.getValue( dah );
		}
		if ( freqfld_->filterType() != FFTFilter::LowPass )
		    outplog.setMnemonicLabel( nullptr );

	    }
	    else if ( act == 2 )
	    {
		Smoother1D<float> sm;
		sm.setInput( inp, sz );
		sm.setOutput( outp );
		const int winsz = gatefld_->getIntValue();
		sm.setWindow( HanningWindow::sName(), 0.95, winsz );
		if ( !sm.execute() )
		    mAddErrMsg( "Could not apply the smoothing window", wllnm )

	    }
	    else if ( act == 3 )
	    {
		Interval<float> rg;
		const float rate = gatefld_->getFValue() / 100.f;
		DataClipSampler dcs( sz );
		dcs.add( outp, sz );
		rg = dcs.getRange( rate );
		for ( int idx=0; idx<sz; idx++ )
		{
		    if ( outp[idx] < rg.start ) outp[idx] = rg.start;
		    if ( outp[idx] > rg.stop )  outp[idx] = rg.stop;
		}
	    }
	    else if ( act == 4 )
	    {
		StepInterval<float> rg( inplog.dahRange() );
		rg.step = gatefld_->getFValue();
		Well::Log* upscaledlog = inplog.upScaleLog( rg );
		outplog = *upscaledlog;
	    }
	    else if ( act == 5 )
	    {
		StepInterval<float> rg( inplog.dahRange() );
		rg.step = gatefld_->getFValue();
		Well::Log* sampledlog = inplog.sampleLog( rg );
		outplog = *sampledlog;
	    }
	    else if ( act == 6 )
	    {
		Well::Log* cleanudflog = inplog.cleanUdfs();
		outplog = *cleanudflog;
	    }
	    outplog.updateAfterValueChanges();
	}
    }

    okbut_->setSensitive( emsg.isEmpty() );
    if ( !emsg.isEmpty() )
	uiMSG().error( mToUiStringTodo(emsg) );

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

    uiStringSet errmsgs;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	WellLogToolData& ld = *logdatas_[idx];
	const Well::LogSet& ls = ld.logs();

	ObjectSet<Well::Log>& outputlogs = ld.outpLogs();
	for ( auto* log : outputlogs )
	{
	    BufferString newnm( log->name() );
	    newnm += extfld_->text();
	    if ( !overwrite && ls.isPresent(newnm) )
	    {
		uiMSG().error(
		    tr("One or more logs with this name already exists."
		    "\nPlease select a different extension for the new logs"));
		return false;
	    }

	    if ( !overwrite )
		log->setName( newnm.buf() );
	}

	const bool res = Well::MGR().writeAndRegister( ld.wellID(),
						       outputlogs );
	if ( !res )
	    errmsgs.add( toUiString(Well::MGR().errMsg()) );
    }

    if ( !errmsgs.isEmpty() )
    {
	errmsgs.insert( 0, tr("Error saving edited logs") );
	uiMSG().errorWithDetails( errmsgs );
    }

    return true;
}


// uiWellLogEditor
uiWellLogEditor::uiWellLogEditor( uiParent* p, Well::Log& log )
    : uiDialog(p,Setup(tr("Edit Well log"),mNoDlgTitle,
		       mODHelpKey(mWellLogEditorHelpID)))
    , log_(log)
    , changed_(false)
    , valueChanged(this)
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
	table_->setValue( RowCol(idx,0), md, 4 );
	table_->setValue( RowCol(idx,1), log_.value(idx) );
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
