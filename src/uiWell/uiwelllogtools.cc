/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelllogtools.cc,v 1.1 2011-01-21 16:02:36 cvsbruno Exp $";

#include "uiwelllogtools.h"

#include "color.h"
#include "statruncalc.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltransl.h"
#include "wellreader.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
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
    ObjectSet<Well::Log> logs; TypeSet<MultiID> wellids;
    welllogselfld_->getSelWellIDs( wellids );
    for ( int idx=0; idx<wellids.size(); idx++ )
    {
	const MultiID& wid = wellids[idx];
	const char* nm = Well::IO::getMainFileName( wid );
	if ( !nm || !*nm ) continue;

	Well::Data wd; Well::Reader wr( nm, wd ); wr.getLogs();
	BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
	for ( int idlog=0; idlog<lognms.size(); idlog ++ )
	    logs += new Well::Log( *wd.logs().getLog( lognms[idlog]->buf() ) );
    }
    uiWellLogToolWin* win = new uiWellLogToolWin( this, logs );
    win->show();

    return false;
}



uiWellLogToolWin::uiWellLogToolWin( uiParent* p, ObjectSet<Well::Log>& logs )
    : uiMainWin(p,"Log Tools Window")
    , logs_(logs)  
{
    uiGroup* loggrp = new uiGroup( this, "Logs group" );
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	uiWellLogDisplay::Setup su;
	uiWellLogDisplay* ld = new uiWellLogDisplay( loggrp, su );
	ld->setPrefWidth( 200 ); ld->setPrefHeight( 500 ); 
	uiWellLogDisplay::LogData& wld = ld->logData( true );
	wld.wl_ = logs_[idx];
	wld.disp_.color_ = Color::stdDrawColor( 1 );
	wld.zoverlayval_ = 1;
	logdisps_ += ld;
	if ( idx ) ld->attach( rightOf, logdisps_[idx-1] );
    }

    uiLabeledComboBox* llc = new uiLabeledComboBox( this, "Action" );
    actionfld_ = llc->box();
    BufferStringSet acts;
    acts.add( "Median Filter" );
    acts.add( "Other ..." );
    actionfld_->addItems( acts );
    llc->attach( ensureBelow, loggrp );

    CallBack cb( mCB( this, uiWellLogToolWin, applyPushedCB ) );
    applybut_ = new uiPushButton( this, "Apply", cb, true );
    applybut_->attach( rightOf, llc );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, llc );

    uiPushButton* okbut = new uiPushButton( this, "&Ok",
				mCB(this,uiWellLogToolWin,acceptOK), true );
    okbut->attach( leftBorder, 20 );
    okbut->attach( ensureBelow, horSepar );

    uiLabel* savelbl = new uiLabel( this, "On OK save logs " );
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
}


uiWellLogToolWin::~uiWellLogToolWin()
{
    deepErase( logs_ );
    deepErase( outplogs_ );
}


void  uiWellLogToolWin::overWriteCB(CallBacker*)
{
    savefld_->setSensitive( !overwritefld_->isChecked() );
}


bool uiWellLogToolWin::acceptOK( CallBacker* )
{ return true; }


bool uiWellLogToolWin::rejectOK( CallBacker* )
{ close(); return true; }


//TODO change ( just took the one from the tutWellTools as an example .. 
void uiWellLogToolWin::applyPushedCB( CallBacker* )
{
    outplogs_.erase();
    const int inpgate = 20;
    for ( int idlog=0; idlog<logs_.size(); idlog++ )
    {
	const Well::Log& inplog = *logs_[idlog];
	Well::Log* outplog = new Well::Log( inplog.name() );
	const int gate = inpgate % 2 ? inpgate : inpgate + 1;
	const int rad = gate / 2;
	Stats::WindowedCalc<float> wcalc(
	Stats::RunCalcSetup().require(Stats::Median), gate );
	const int sz = inplog.size();
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
	outplogs_ += outplog;
    }
    displayOutpLogs( 0 );
}


void uiWellLogToolWin::displayOutpLogs( CallBacker* )
{
    for ( int idx=0; idx<outplogs_.size(); idx++ )
    {
	uiWellLogDisplay* ld = logdisps_[idx];
	uiWellLogDisplay::LogData& wld = ld->logData( false );
	wld.wl_ = outplogs_[idx];
	wld.xrev_ = false;
	wld.zoverlayval_ = 2;
	wld.disp_.color_ = Color::stdDrawColor( 0 );
	ld->reDraw();
    }
}


