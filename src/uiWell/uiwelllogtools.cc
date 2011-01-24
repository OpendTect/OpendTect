/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelllogtools.cc,v 1.2 2011-01-24 16:43:46 cvsbruno Exp $";

#include "uiwelllogtools.h"

#include "color.h"
#include "statruncalc.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uiwelllogdisplay.h"


uiWellLogToolWinMgr::uiWellLogToolWinMgr( uiParent* p )
	: uiDialog( p, Setup( "Well log tool", 
		    "Select logs to be processed", mTODOHelpID ) )
{
    setCtrlStyle( DoAndStay );
    welllogselfld_ = new uiMultiWellLogSel( this );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiWellLogToolWinMgr::acceptOK( CallBacker* )
{
    BufferStringSet wellids; welllogselfld_->getSelWellIDs( wellids );
    if ( wellids.isEmpty() ) mErrRet( "Please select at least one well" )

    ObjectSet<uiWellLogToolWin::LogData> logdatas; 
    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const MultiID& wid = wellids[idx]->buf();
	const char* nm = Well::IO::getMainFileName( wid );
	if ( !nm || !*nm ) continue;

	Well::Data wd; Well::Reader wr( nm, wd ); wr.getLogs(); wr.getMarkers();
	BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
	Well::LogSet* wls = new Well::LogSet( wd.logs() );
	uiWellLogToolWin::LogData* ldata = new uiWellLogToolWin::LogData(*wls);
	if ( !ldata->setSelectedLogs( lognms ) ) 
	    { delete ldata; continue; }
	ldata->wellname_ = nm; 
	Interval<float> dahrg; 
	const char* topm; const char* botm; float topd, botd;
	welllogselfld_->getLimitMarkers( topm, botm );
	welllogselfld_->getLimitDists( topd, botd );
	const Well::Marker* topmrk = wd.markers().getByName( topm );
	const Well::Marker* botmrk = wd.markers().getByName( botm );
	ldata->dahrg_.start = topmrk ? topmrk->dah() : wls->dahInterval().start;
	ldata->dahrg_.stop  = botmrk ? topmrk->dah() : wls->dahInterval().stop;
	ldata->dahrg_.start -= topd; 	
	ldata->dahrg_.stop += botd;

	logdatas += ldata;
    }
    if ( logdatas.isEmpty() )
	mErrRet("Please select at least one valid log for the selected well(s)")

    uiWellLogToolWin* win = new uiWellLogToolWin( this, logdatas );
    win->show();
    win->windowClosed.notify( mCB( this, uiWellLogToolWinMgr, winClosed ) );

    return false;
}


void uiWellLogToolWinMgr::winClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellLogToolWin*,win,cb)
    if ( !win ) pErrMsg( "can not find window" );
    if ( win->needSave() )
    {
	ObjectSet<uiWellLogToolWin::LogData> lds; win->getLogDatas( lds );
	for ( int idx=0; idx<lds.size(); idx++ )
	{
	    Well::Data wd; lds[idx]->getOutputLogs( wd.logs() );
	    Well::Writer wrr( lds[idx]->wellname_, wd );
	    wrr.putLogs();
	}
	welllogselfld_->update();
    }
}



uiWellLogToolWin::LogData::LogData( const Well::LogSet& ls )
    : logs_(*new Well::LogSet)
{
    for ( int idx=0; idx<ls.size(); idx++ )
	logs_.add( new Well::Log( ls.getLog( idx ) ) );
}


uiWellLogToolWin::LogData::~LogData()
{
    deepErase( outplogs_ );
    delete &logs_;
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
	if ( wl ) { inplogs_ += wl; nrsel++; }
    }
    return nrsel;
}



uiWellLogToolWin::uiWellLogToolWin( uiParent* p, ObjectSet<LogData>& logs )
    : uiMainWin(p,"Log Tools Window")
    , logdatas_(logs)
    , needsave_(false)
{
    uiGroup* loggrp = new uiGroup( this, "Logs group" );
    zdisplayrg_ = logdatas_[0]->dahrg_; 
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& logdata = *logdatas_[idx];
	for ( int idlog=0; idlog<logdata.inplogs_.size(); idlog++ )
	{
	    uiWellLogDisplay::Setup su;
	    uiWellLogDisplay* ld = new uiWellLogDisplay( loggrp, su );
	    ld->setPrefWidth( 200 ); ld->setPrefHeight( 500 ); 
	    logdisps_ += ld;
	    zdisplayrg_.include( logdata.dahrg_ );
	    if ( idlog ) ld->attach( rightOf, logdisps_[idlog-1] );
	}
    }

    loggrp->setHSpacing( 0 );
    uiLabeledComboBox* llc = new uiLabeledComboBox( this, "Action" );
    actionfld_ = llc->box();
    BufferStringSet acts;
    acts.add( "Median filter" );
    acts.add( "Remove spikes" );
    actionfld_->addItems( acts );
    llc->attach( ensureBelow, loggrp );
    llc->attach( hCentered );

    CallBack cb( mCB( this, uiWellLogToolWin, applyPushedCB ) );
    applybut_ = new uiPushButton( this, "Apply", cb, true );
    applybut_->attach( rightOf, llc );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, llc );

    uiPushButton* okbut = new uiPushButton( this, "&Ok",
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut->attach( leftBorder, 20 );
    okbut->attach( ensureBelow, horSepar );

    uiLabel* savelbl = new uiLabel( this, "On OK save logs: " );
    savelbl->attach( rightOf, okbut );
    savefld_ = new uiGenInput( this, "with extension " );
    savefld_->setElemSzPol( uiObject::Small );
    savefld_->setText( "_edited" );
    savefld_->attach( rightOf, savelbl );
    savefld_->setStretch( 0, 0 );

    overwritefld_ = new uiCheckBox( this, "Overwrite" );
    overwritefld_->attach( rightOf, savefld_ );
    overwritefld_->activated.notify( mCB(this,uiWellLogToolWin,overWriteCB) );
    overwritefld_->setStretch( 0, 0 );

    uiPushButton* cancelbut = new uiPushButton( this, "&Cancel",
				mCB(this,uiWellLogToolWin,rejectOK), true );
    cancelbut->attach( rightBorder );
    cancelbut->attach( ensureBelow, horSepar );
    cancelbut->attach( rightBorder, 20 );
    overwritefld_->attach( ensureLeftOf, cancelbut );

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


bool uiWellLogToolWin::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	LogData& ld = *logdatas_[idx];
	Well::LogSet& ls = ld.logs_;
	bool overwrite = overwritefld_->isChecked();
	for ( int idl=ld.outplogs_.size()-1; idl>=0; idl-- )
	{
	    Well::Log* outplog = ld.outplogs_.remove( idl );
	    if ( overwrite )
	    {
		Well::Log* log = ls.getLog( outplog->name() );
		delete log; log = outplog;
	    }
	    else
	    {
		BufferString newnm( outplog->name() );
		newnm += savefld_->text();
		outplog->setName( newnm );
		ls.add( outplog );
	    }
	    needsave_ = true; 
	}
    }
    close(); return true; 
}


bool uiWellLogToolWin::rejectOK( CallBacker* )
{ close(); return true; }


//TODO change ( just took the one from the tutWellTools as an example .. 
void uiWellLogToolWin::applyPushedCB( CallBacker* )
{
    const int inpgate = 20;
    for ( int idldata=0; idldata<logdatas_.size(); idldata++ )
    {
	LogData& ld = *logdatas_[idldata]; deepErase( ld.outplogs_ );
	for ( int idlog=0; idlog<ld.inplogs_.size(); idlog++ )
	{
	    const Well::Log& inplog = *ld.inplogs_[idlog];
	    Well::Log* outplog = new Well::Log( inplog );
	    outplog->erase();
	    const int sz = inplog.size(); 
	    const int gate = inpgate % 2 ? inpgate : inpgate + 1;
	    const int rad = gate / 2;
	    Stats::WindowedCalc<float> wcalc(
	    Stats::RunCalcSetup().require(Stats::Median), gate );
	    for ( int idx=0; idx<sz+rad; idx++ )
	    {
		const int cpos = idx - rad;
		if ( idx < sz )
		{
		    const float inval = inplog.value(idx);
		    if (!mIsUdf(inval) )
			wcalc += inval;
		    if ( cpos >= rad )
			outplog->addValue( inplog.dah(cpos), wcalc.median() );
		}
		else
		    outplog->addValue( inplog.dah(cpos), inplog.value(cpos) );

		if ( cpos<rad && cpos>=0 )
		    outplog->addValue( inplog.dah(cpos), inplog.value(cpos) );
	    }
	    ld.outplogs_ += outplog;
	}
    }
    displayLogs();
}


void uiWellLogToolWin::displayLogs()
{
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	ObjectSet<Well::Log>& inplogs = logdatas_[idx]->inplogs_;
	ObjectSet<Well::Log>& outplogs = logdatas_[idx]->outplogs_;
	for ( int idlog=0; idlog<inplogs.size(); idlog++ )
	{
	    uiWellLogDisplay* ld = logdisps_[idlog];
	    uiWellLogDisplay::LogData* wld = &ld->logData( true );
	    wld->wl_ = inplogs[idlog];
	    wld->disp_.color_ = Color::stdDrawColor( 1 );
	    wld->zoverlayval_ = 1;

	    wld = &ld->logData( false );
	    wld->wl_ = outplogs.validIdx( idlog ) ? outplogs[idx] : 0;
	    wld->xrev_ = false;
	    wld->zoverlayval_ = 2;
	    wld->disp_.color_ = Color::stdDrawColor( 0 );

	    ld->setZRange( zdisplayrg_ );
	    ld->reDraw();
	}
    }
}


