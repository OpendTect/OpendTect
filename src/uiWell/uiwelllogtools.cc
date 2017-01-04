/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwelllogtools.h"

#include "arrayndimpl.h"
#include "color.h"
#include "dataclipper.h"
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
#include "welltransl.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifreqfilter.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uiwelllogdisplay.h"


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


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiWellLogToolWinMgr::acceptOK( CallBacker* )
{
    BufferStringSet wellids; welllogselfld_->getSelWellIDs( wellids );
    BufferStringSet wellnms; welllogselfld_->getSelWellNames( wellnms );
    if ( wellids.isEmpty() ) mErrRet( tr("Please select at least one well") )

    ObjectSet<uiWellLogToolWin::LogData> logdatas;
    BufferStringSet msgs;
    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const MultiID& wmid = wellids[idx]->buf();
	RefMan<Well::Data> wd = Well::MGR().get( wmid );
	if ( !wd )
	{
	    msgs += new BufferString( Well::MGR().errMsg() );
	    continue;
	}

	BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
	Well::LogSet* wls = new Well::LogSet( wd->logs() );
	uiWellLogToolWin::LogData* ldata =
	    new uiWellLogToolWin::LogData( *wls, wd->d2TModel(), &wd->track());
	const Well::ExtractParams& params = welllogselfld_->params();
	ldata->dahrg_ = params.calcFrom( *wd, lognms, true );
	ldata->wellname_ = wellnms[idx]->buf();
	if ( !ldata->setSelectedLogs( lognms ) )
	    { delete ldata; continue; }
	ldata->wellid_ = wmid;

	logdatas += ldata;
    }

    if ( logdatas.isEmpty() )
	mErrRet(tr("%1\nPlease select at least one valid "
		   "log for the selected well(s)")
		   .arg(msgs.cat()) )
    else if ( !msgs.isEmpty() )
	uiMSG().warning( tr("%1\nWill process the other wells only")
			     .arg( msgs.cat() ) );

    uiWellLogToolWin* win = new uiWellLogToolWin( this, logdatas );
    win->show();
    win->windowClosed.notify( mCB(this,uiWellLogToolWinMgr,winClosed) );

    return false;
}


void uiWellLogToolWinMgr::winClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellLogToolWin*,win,cb)
    if ( !win )
	{ pErrMsg( "cb null or not uiWellLogToolWin" ); return; }

    if ( win->needSave() )
    {
	ObjectSet<uiWellLogToolWin::LogData> lds; win->getLogDatas( lds );
	for ( int idx=0; idx<lds.size(); idx++ )
	{
	    RefMan<Well::Data> wd = new Well::Data;
	    lds[idx]->getOutputLogs( wd->logs() );
	    Well::Writer wrr( lds[idx]->wellid_, *wd );
	    wrr.putLogs();
	    Well::MGR().reload( lds[idx]->wellid_ );
	}
	welllogselfld_->update();
    }
}



uiWellLogToolWin::LogData::LogData( const Well::LogSet& ls,
				    const Well::D2TModel* d2t,
				    const Well::Track* track )
    : logs_(*new Well::LogSet)
{
    d2t_ = d2t ? new Well::D2TModel(*d2t) : 0;
    track_ = track ? new Well::Track(*track) : 0;
    for ( int idx=0; idx<ls.size(); idx++ )
	logs_.add( new Well::Log( ls.getLog( idx ) ) );
}


uiWellLogToolWin::LogData::~LogData()
{
    deepErase( outplogs_ );
    delete &logs_;
    delete d2t_;
}


void uiWellLogToolWin::LogData::getOutputLogs( Well::LogSet& ls ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	ls.add( new Well::Log( logs_.getLog( idx ) ) );
}


int uiWellLogToolWin::LogData::setSelectedLogs( BufferStringSet& lognms )
{
    int nrsel = 0;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::Log* wl = logs_.getLog( lognms[idx]->buf() );
	if ( !wl || wl->isEmpty() )
	    continue;
	for ( int dahidx=wl->size()-1; dahidx>=0; dahidx -- )
	{
	    if ( !dahrg_.includes( wl->dah( dahidx ), true ) )
		wl->remove( dahidx );
	}
	inplogs_ += wl;
	nrsel++;
    }
    return nrsel;
}



uiWellLogToolWin::uiWellLogToolWin( uiParent* p, ObjectSet<LogData>& logs )
    : uiMainWin(p,tr("Log Tools Window"))
    , logdatas_(logs)
    , needsave_(false)
{
    uiGroup* displaygrp = new uiGroup( this, "Well display group" );
    displaygrp->setHSpacing( 0 );
    zdisplayrg_ = logdatas_[0]->dahrg_;
    uiGroup* wellgrp; uiGroup* prevgrp = 0; uiLabel* wellnm;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& logdata = *logdatas_[idx];
	wellgrp  = new uiGroup( displaygrp, "Well group" );
	if ( prevgrp ) wellgrp->attach( rightOf, prevgrp );
	wellgrp->setHSpacing( 0 );
	wellnm = new uiLabel( wellgrp, toUiString(logdata.wellname_) );
	wellnm->setVSzPol( uiObject::Small );
	for ( int idlog=0; idlog<logdata.inplogs_.size(); idlog++ )
	{
	    uiWellLogDisplay::Setup su; su.samexaxisrange_ = true;
	    uiWellLogDisplay* ld = new uiWellLogDisplay( wellgrp, su );
	    ld->setPrefWidth( 150 ); ld->setPrefHeight( 450 );
	    zdisplayrg_.include( logdata.dahrg_ );
	    if ( idlog ) ld->attach( rightOf, logdisps_[logdisps_.size()-1] );
	    ld->attach( ensureBelow, wellnm );
	    logdisps_ += ld;
	}
	prevgrp = wellgrp;
    }
    zdisplayrg_.sort();

    uiGroup* actiongrp = new uiGroup( this, "Action" );
    actiongrp->attach( hCentered );
    actiongrp->attach( ensureBelow, displaygrp );
    const char* acts[] = { "Remove Spikes", "FFT Filter", "Smooth", "Clip", 0 };
    uiLabeledComboBox* llc = new uiLabeledComboBox( actiongrp, acts,
						    uiStrings::sAction() );
    actionfld_ = llc->box();
    actionfld_->selectionChanged.notify(mCB(this,uiWellLogToolWin,actionSelCB));

    CallBack cb( mCB( this, uiWellLogToolWin, applyPushedCB ) );
    applybut_ = uiButton::getStd( actiongrp, OD::Apply, cb, true );
    applybut_->attach( rightOf, llc );

    freqfld_ = new uiFreqFilterSelFreq( actiongrp );
    freqfld_->attach( alignedBelow, llc );

    uiLabeledSpinBox* spbgt = new uiLabeledSpinBox( actiongrp,
						    tr("Window size") );
    spbgt->attach( alignedBelow, llc );
    gatefld_ = spbgt->box();
    gatelbl_ = spbgt->label();

    const uiString txt = tr("Threshold ( Grubbs number )");
    thresholdfld_ = new uiLabeledSpinBox( actiongrp, txt );
    thresholdfld_->attach( rightOf, spbgt );
    thresholdfld_->box()->setInterval( 1.0, 20.0, 0.1 );
    thresholdfld_->box()->setValue( 3 );
    thresholdfld_->box()->setNrDecimals( 2 );

    const char* spk[] = {"Undefined values","Interpolated values","Specify",0};
    replacespikefld_ = new uiLabeledComboBox(actiongrp,spk,
					     tr("Replace spikes by"));
    replacespikefld_->box()->selectionChanged.notify(
				mCB(this,uiWellLogToolWin,handleSpikeSelCB) );
    replacespikefld_->attach( alignedBelow, spbgt );

    replacespikevalfld_ = new uiGenInput( actiongrp, uiStrings::sEmptyString(),
							       FloatInpSpec() );
    replacespikevalfld_->attach( rightOf, replacespikefld_ );
    replacespikevalfld_->setValue( 0 );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, actiongrp );

    okbut_ = uiButton::getStd( this, OD::Ok,
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut_->attach( leftBorder, 20 );
    okbut_->attach( ensureBelow, horSepar );
    okbut_->setSensitive( false );

    uiLabel* savelbl = new uiLabel( this, tr("On OK save logs:") );
    savelbl->attach( rightOf, okbut_ );
    savefld_ = new uiGenInput( this, tr("with extension") );
    savefld_->setElemSzPol( uiObject::Small );
    savefld_->setText( "_edited" );
    savefld_->attach( rightOf, savelbl );
    savefld_->setStretch( 0, 0 );

    overwritefld_ = new uiCheckBox( this, uiStrings::sOverwrite() );
    overwritefld_->attach( rightOf, savefld_ );
    overwritefld_->activated.notify( mCB(this,uiWellLogToolWin,overWriteCB) );
    overwritefld_->setStretch( 0, 0 );

    uiButton* cancelbut = uiButton::getStd( this, OD::Cancel,
				mCB(this,uiWellLogToolWin,rejectOK), true );
    cancelbut->attach( rightBorder, 20 );
    cancelbut->attach( ensureBelow, horSepar );
    cancelbut->attach( ensureRightOf, overwritefld_ );

    actionSelCB(0);
    displayLogs();
}


uiWellLogToolWin::~uiWellLogToolWin()
{
    deepErase( logdatas_ );
}


void  uiWellLogToolWin::overWriteCB(CallBacker*)
{
    savefld_->setSensitive( !overwritefld_->isChecked() );
}


void  uiWellLogToolWin::actionSelCB( CallBacker* )
{
    const int act = actionfld_->currentItem();

    thresholdfld_->display( act == 0 );
    replacespikevalfld_->display( act == 0 );
    replacespikefld_->display( act == 0 );
    freqfld_->display( act == 1 );
    gatefld_->display( act != 1 );
    gatelbl_->display( act != 1 );
    gatelbl_->setText( act > 2 ? tr("Clip rate (%)")
			       : tr("Window size (samples)") );
    StepInterval<int> sp = act > 2 ? StepInterval<int>(0,100,10)
				   : StepInterval<int>(1,1500,5);
    gatefld_->setInterval( sp );
    gatefld_->setValue( act > 2 ? 1 : 300 );

    handleSpikeSelCB(0);
}


void uiWellLogToolWin::handleSpikeSelCB( CallBacker* )
{
    const int act = replacespikefld_->box()->currentItem();
    replacespikevalfld_->display( act == 2 );
}


bool uiWellLogToolWin::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& ld = *logdatas_[idx];
	Well::LogSet& ls = ld.logs_;
	bool overwrite = overwritefld_->isChecked();
	for ( int idl=ld.outplogs_.size()-1; idl>=0; idl-- )
	{
	    Well::Log* outplog = ld.outplogs_.removeSingle( idl );
	    if ( overwrite )
	    {
		const int logidx = ls.indexOf( outplog->name() );
		if ( ls.validIdx( logidx ) )
		    delete ls.remove( logidx );
	    }
	    else
	    {
		BufferString newnm( outplog->name() );
		newnm += savefld_->text();
		outplog->setName( newnm );
		if ( ls.getLog( outplog->name() ) )
		{
		    mErrRet(tr("One or more logs with this name already exists."
		    "\nPlease select a different extension for the new logs"));
		}
	    }
	    ls.add( outplog );
	    needsave_ = true;
	}
    }
    close(); return true;
}


bool uiWellLogToolWin::rejectOK( CallBacker* )
{ close(); return true; }


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
	LogData& ld = *logdatas_[idldata];
	deepErase( ld.outplogs_ );
	const BufferString wllnm = ld.wellname_;
	for ( int idlog=0; idlog<ld.inplogs_.size(); idlog++ )
	{
	    const Well::Log& inplog = *ld.inplogs_[idlog];
	    Well::Log* outplog = new Well::Log( inplog );
	    const int sz = inplog.size();
	    const int gate = gatefld_->getValue();
	    if ( sz< 2 || ( act != 1 && sz < 2*gate ) ) continue;

	    ld.outplogs_ += outplog;
	    const float* inp = inplog.valArr();
	    float* outp = outplog->valArr();
	    if ( act == 0 )
	    {
		Stats::Grubbs sgb;
		const float cutoff_grups = mCast( float,
					    thresholdfld_->box()->getValue() );
		TypeSet<int> grubbsidxs;
		mAllocVarLenArr( float, gatevals, gate )
		for ( int idx=gate/2; idx<sz-gate; idx+=gate  )
		{
		    float cutoffval = cutoff_grups + 1;
		    while ( cutoffval > cutoff_grups )
		    {
			for (int winidx=0; winidx<gate; winidx++)
			    gatevals[winidx]= outp[idx+winidx-gate/2];

			int idxtofix;
			cutoffval = sgb.getMax( mVarLenArr(gatevals),
						gate, idxtofix ) ;
			if ( cutoffval > cutoff_grups  && idxtofix >= 0 )
			{
			    outp[idx+idxtofix-gate/2] = mUdf( float );
			    grubbsidxs += idx+idxtofix-gate/2;
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
			grval = replacespikevalfld_->getfValue();
		    }
		    else if ( spkact == 1 )
		    {
			float dah = outplog->dah( gridx );
			grval = outplog->getValue( dah, true );
		    }
		}
	    }
	    else if ( act == 1)
	    {
		const Well::Track& track = *ld.track_;
		const float startdah = outplog->dahRange().start;
		const float stopdah = outplog->dahRange().stop;
		const float zstart = mCast( float, track.getPos( startdah ).z );
		const float zstop = mCast( float, track.getPos( stopdah ).z );
		const Interval<float> zrg( zstart, zstop );
		ObjectSet<const Well::Log> reslogs;
		reslogs += outplog;
		Stats::UpscaleType ut = Stats::UseAvg;
		const float deftimestep = 0.001f;
		Well::LogSampler ls( ld.d2t_, &track, zrg, false, deftimestep,
				     SI().zIsTime(), ut, reslogs );
		if ( !ls.execute() )
		    mAddErrMsg( "Could not resample the logs", wllnm )

		const int size = ls.nrZSamples();
		Array1DImpl<float> logvals( size );
		for ( int idz=0; idz<size; idz++ )
		    logvals.set( idz, ls.getLogVal( idlog, idz ) );

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
		    const float dah = outplog->dah( idz );
		    outp[idz] = filtvals.getValue( dah );
		}
	    }
	    else if ( act == 2 )
	    {
		Smoother1D<float> sm;
		sm.setInput( inp, sz );
		sm.setOutput( outp );
		const int winsz = gatefld_->getValue();
		sm.setWindow( HanningWindow::sName(), 0.95, winsz );
		if ( !sm.execute() )
		    mAddErrMsg( "Could not apply the smoothing window", wllnm )
	    }
	    else if ( act == 3 )
	    {
		Interval<float> rg;
		const float rate = gate / 100.f;
		DataClipSampler dcs( sz );
		dcs.add( outp, sz );
		rg = dcs.getRange( rate );
		for ( int idx=0; idx<sz; idx++ )
		{
		    if ( outp[idx] < rg.start ) outp[idx] = rg.start;
		    if ( outp[idx] > rg.stop )  outp[idx] = rg.stop;
		}
	    }
	}
    }
    okbut_->setSensitive( emsg.isEmpty() );
    if ( !emsg.isEmpty() )
	uiMSG().error( mToUiStringTodo(emsg) );

    displayLogs();
}


void uiWellLogToolWin::displayLogs()
{
    int nrdisp = 0;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	ObjectSet<const Well::Log>& inplogs = logdatas_[idx]->inplogs_;
	ObjectSet<Well::Log>& outplogs = logdatas_[idx]->outplogs_;
	for ( int idlog=0; idlog<inplogs.size(); idlog++ )
	{
	    uiWellLogDisplay* ld = logdisps_[nrdisp];
	    uiWellLogDisplay::LogData* wld = &ld->logData( true );
	    wld->setLog( inplogs[idlog] );
	    wld->disp_.color_ = Color::stdDrawColor( 1 );
	    wld->zoverlayval_ = 1;

	    wld = &ld->logData( false );
	    wld->setLog( outplogs.validIdx( idlog ) ? outplogs[idlog] : 0 );
	    wld->xrev_ = false;
	    wld->zoverlayval_ = 2;
	    wld->disp_.color_ = Color::stdDrawColor( 0 );

	    ld->setZRange( zdisplayrg_ );
	    ld->reDraw();
	    nrdisp ++;
	}
    }
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
    table_->valueChanged.notify( mCB(this,uiWellLogEditor,valChgCB) );
    table_->rowDeleted.notify( mCB(this,uiWellLogEditor,rowDelCB) );
    table_->selectionDeleted.notify( mCB(this,uiWellLogEditor,rowDelCB) );
    table_->rowInserted.notify( mCB(this,uiWellLogEditor,rowInsertCB) );
    BufferString mdlbl( "MD" );
    mdlbl.add( getDistUnitString(SI().depthsInFeet(), true) );
    BufferString loglbl( log_.name() );
    if ( log_.unitMeasLabel() && *log_.unitMeasLabel() )
	loglbl.add( "(" ).add( log_.unitMeasLabel() ).add( ")" );

    BufferStringSet colnms; colnms.add(mdlbl).add(loglbl);
    table_->setColumnLabels( colnms );

    fillTable();
}


uiWellLogEditor::~uiWellLogEditor()
{
}


void uiWellLogEditor::fillTable()
{
    NotifyStopper ns( table_->valueChanged );
    const int sz = log_.size();
    const UnitOfMeasure* depthunit = UnitOfMeasure::surveyDefDepthUnit();

    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = depthunit->getUserValueFromSI( log_.dah(idx) );
	table_->setValue( RowCol(idx,0), val );
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
    const float newval = table_->getfValue( rc );
    const float oldval = mdchanged ? log_.dah( rc.row() ) : 
							log_.value( rc.row() );
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
    TypeSet<int> rowidxset = table_->getNotifRCs();
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
    int rownr = table_->currentRow();
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
