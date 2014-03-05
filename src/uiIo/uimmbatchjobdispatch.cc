/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimmbatchjobdispatch.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uislider.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uitextfile.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"
#include "uimsg.h"
#include "timer.h"
#include "timefun.h"
#include "oddirs.h"
#include "envvars.h"
#include "hostdata.h"
#include "batchjobdispatch.h"
class HostDataList;



uiMMBatchJobDispatcher::uiMMBatchJobDispatcher( uiParent* p,
						const Batch::JobSpec& js )
	: uiDialog(p,uiDialog::Setup("",mNoDlgTitle,mNoHelpID)
		.nrstatusflds(-1)
		.fixedsize(true))
	, jobspec_(*new Batch::JobSpec(js))
	, hdl_(*new HostDataList)
	, avmachfld_(0), usedmachfld_(0)
	, nicefld_(0)
	, logvwer_(0)
	, progrfld_(0), progbar_(0)
	, jrpstartfld_(0), jrpstopfld_(0)
	, timer_(0)
	, nrcyclesdone_(0)
	, basecaption_("Job management")
{
    setCaption( basecaption_ );

    const int nrhosts = hdl_.size();
    const bool multihost = nrhosts > 1;
    int maxhostdisp = 1;
    if ( multihost )
	maxhostdisp = nrhosts>7 ? 8 : (nrhosts<3 ? 3 : nrhosts);
    const int hostnmwdth = 30;

    statusBar()->addMsgFld( "Message", Alignment::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", Alignment::Right, 20 );
    statusBar()->addMsgFld( "NrDone", Alignment::Left, 10 );
    statusBar()->addMsgFld( "Activity", Alignment::Left, 1 );

    uiGroup* specparsgroup_ = new uiGroup( this, "Specific parameters group" );
    uiSeparator* sep = new uiSeparator( this, "Hor sep 1" );
    sep->attach( stretchedBelow, specparsgroup_ );

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    uiLabeledListBox* avmachfld = 0;
    if ( multihost )
    {
	avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
					  uiLabeledListBox::AboveMid );
	machgrp->setHAlignObj( avmachfld );
	avmachfld_ = avmachfld->box();
	for ( int idx=0; idx<hdl_.size(); idx++ )
	{
	    const HostData& hd = *hdl_[idx];
	    BufferString nm( hd.name() );
	    const int nraliases = hd.nrAliases();
	    for ( int aliasidx=0; aliasidx<nraliases; aliasidx++ )
		{ nm += " / "; nm += hd.alias(aliasidx); }
	    avmachfld_->addItem( nm );
	}

	avmachfld_->setPrefWidthInChar( mCast(float,hostnmwdth) );
	avmachfld_->setPrefHeightInChar( mCast(float,maxhostdisp) );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    uiLabeledListBox* usedmachfld = new uiLabeledListBox( usedmachgrp,
				    multihost ? "Used hosts" : "", false,
				    uiLabeledListBox::AboveMid );
    usedmachfld_ = usedmachfld->box();
    usedmachfld_->setPrefWidthInChar( hostnmwdth );
    usedmachfld_->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp, "St&op", true );
    stopbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp, "&View log", false );
    vwlogbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld );
	addbut = new uiPushButton( machgrp, ">> &Add >>", true );
	if ( avmachfld )
	    addbut->attach( centeredRightOf, avmachfld );
	usedmachgrp->attach( ensureRightOf, addbut );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "St&art", true );
	addbut->attach( alignedBelow, usedmachfld );
	stopbut->attach( centeredBelow, usedmachfld );
	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,addPush) );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld_ = new uiSlider( jrppolgrp, "Nice level" );
    nicefld_->setMinValue( -0.5 ); nicefld_->setMaxValue( 19.5 );
    nicefld_->setValue( hdl_.defNiceLevel() );
    uiLabel* nicelbl = new uiLabel( jrppolgrp, "'Nice' level (0-19)" );
    nicelbl->attach( rightOf, nicefld_ );
    if ( avmachfld_ )
	nicefld_->setPrefWidthInChar( hostnmwdth );

    jrppolselfld_ = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld_->addItem( "Run" );
    jrppolselfld_->addItem( "Pause" );
    jrppolselfld_->addItem( "Go - Only between" );
    jrppolselfld_->setCurrentItem( ((int)0) );
    jrppolselfld_->selectionChanged.notify(
				mCB(this,uiMMBatchJobDispatcher,jrpSel) );
    jrppolselfld_->attach( alignedBelow, nicefld_ );
    if ( avmachfld_ ) jrppolselfld_->setPrefWidthInChar( hostnmwdth );
    jrpworklbl_ = new uiLabel( jrppolgrp, "Processes" );
    jrpworklbl_->attach( rightOf, jrppolselfld_ );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );
    jrpstartfld_ = new uiGenInput( jrppolgrp, "", envstr ? envstr : "18:00" );
    jrpstartfld_->attach( rightOf, jrppolselfld_ );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld_ = new uiGenInput( jrppolgrp, "and", envstr ? envstr : "7:30" );
    jrpstopfld_->attach( rightOf, jrpstartfld_ );

    jrppolgrp->setHAlignObj( nicefld_ );
    jrppolgrp->attach( ensureBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2" );
    sep->attach( stretchedBelow, jrppolgrp );

    progrfld_ = new uiTextEdit( this, "Processing progress", true );
    progrfld_->attach( ensureBelow, sep );
    progrfld_->attach( widthSameAs, sep );
    progrfld_->setPrefHeightInChar( 7 );

    progbar_ = new uiProgressBar( this, "", 1, 0 );
    progbar_->attach( widthSameAs, progrfld_ );
    progbar_->attach( alignedBelow, progrfld_ );

    postFinalise().notify( mCB(this,uiMMBatchJobDispatcher,initWin) );
}


uiMMBatchJobDispatcher::~uiMMBatchJobDispatcher()
{
    delete logvwer_;
    delete timer_;

    delete &jobspec_;
    delete &hdl_;
}


void uiMMBatchJobDispatcher::initWin( CallBacker* cb )
{
    jrpSel( cb );
    if ( !avmachfld_ )
	addPush( cb );
}


void uiMMBatchJobDispatcher::startWork( CallBacker* )
{
    setOkText( "Finish Now" );
    setCancelText( "Abort" );

    timer_ = new Timer("uiMMBatchJobDispatcher timer");
    timer_->tick.notify( mCB(this,uiMMBatchJobDispatcher,doCycle) );
    timer_->start( 100, true );
}


void uiMMBatchJobDispatcher::doCycle( CallBacker* )
{
    nrcyclesdone_++;
    timer_->start( 250, true );
}


void uiMMBatchJobDispatcher::stopPush( CallBacker* )
{
}


void uiMMBatchJobDispatcher::vwLogPush( CallBacker* )
{
}


void uiMMBatchJobDispatcher::jrpSel( CallBacker* )
{
    const bool isgo = *jrppolselfld_->text() == 'G';
    jrpstartfld_->display( isgo );
    jrpstopfld_->display( isgo );
    jrpworklbl_->display( !isgo );
}


void uiMMBatchJobDispatcher::updateAliveDisp()
{
    const int nrdispstrs = 6;
    const char* dispstrs[]
	= { ">..", ".>.", "..>", "..<", ".<.", "<.." };
    statusBar()->message( dispstrs[ nrcyclesdone_ % nrdispstrs ], 3 );

    const int totsteps = -1;
    const int nrdone = 0;
    const bool hastot = totsteps > 0;
    progbar_->display( hastot );
    if ( hastot )
    {
	progbar_->setTotalSteps( totsteps );
	progbar_->setProgress( nrdone );

	const float fpct = 100.f * ((float)nrdone) / totsteps;
	int pct = (int)fpct; if ( pct > 100 ) pct = 100;
	BufferString newcap( "[" ); newcap += pct; newcap += "%] ";
	newcap += basecaption_;
	setCaption( newcap );
    }
}


#ifdef __win__
#define mAddReDirectToNull checkcmd += " > NUL"
#else
#define mAddReDirectToNull checkcmd += " > /dev/null"
#endif

#ifndef __win__

static bool hostOK( const HostData& hd, const char* rshcomm,
		      BufferString& errmsg )
{
    BufferString remotecmd( rshcomm );
    remotecmd += " "; remotecmd += hd.name();
    BufferString checkcmd( remotecmd ); checkcmd += " whoami";
    mAddReDirectToNull;
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot establish a ";
	errmsg += rshcomm; errmsg += " connection with ";
	errmsg += hd.name();
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Appl, GetSoftwareDir(0) ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find application directory ";
	errmsg += hd.name(); errmsg += ":";
	errmsg += hd.convPath(HostData::Appl, GetSoftwareDir(0)).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host ";
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Data, GetBaseDataDir() ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find data directory ";
	errmsg += hd.name(); errmsg += ":";
	errmsg += hd.convPath(HostData::Data, GetBaseDataDir()).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host";
	return false;
    }

    return true;
}

#endif


#define mErrRet(s) { uiMSG().error(s); return; }

void uiMMBatchJobDispatcher::addPush( CallBacker* )
{
    const int nrmach = avmachfld_ ? avmachfld_->size() : 1;
    if ( nrmach < 1 ) return;
    const int nrsel = avmachfld_ ? avmachfld_->nrSelected() : 1;
    if ( nrsel < 1 )
	mErrRet("Please select one or more hosts")

    for ( int idx=0; idx<nrmach; idx++ )
    {
	if ( avmachfld_ && !avmachfld_->isSelected(idx) ) continue;

	BufferString hnm = avmachfld_ ? avmachfld_->textOfItem( idx )
				      : hdl_[0]->name();
	char* ptr = hnm.find( ' ' );
	if ( ptr ) *ptr = '\0';

	const HostData* hd = hdl_.find( hnm.buf() );
	if ( !hd ) { pErrMsg("Huh"); continue; }

#ifndef __win__
	BufferString errmsg;
	if ( !hd->isKnownAs(HostData::localHostName())
		&& !hostOK(*hd,hdl_.rshComm(),errmsg) )
	{
	    progrfld_->append( errmsg.buf() );
	    continue;
	}
#endif
    }
}


const char* uiMMBatchJobDispatcher::curUsedMachName()
{
   mDeclStaticString( mach );
   mach = usedmachfld_->getText();

   char* ptr = mach.find( " -:- ");
   if ( ptr ) *ptr='\0';

   return mach;
}


static int getSecs( const char* txt )
{
    if ( !txt || !*txt ) return 0;
    BufferString bs( txt );
    char* mid = bs.find( ':' );
    if ( mid ) *mid++ = '\0';

    int secs=-1;
    if ( mid && *mid )
    {
	char* head = mid-1;
	while ( *head != ' ' && head > bs.buf() ) head--;

	char* tail = firstOcc( mid, ':' );
	if ( tail ) *tail++ = '\0';

	secs = toInt( head ) * 3600;
	secs += toInt( mid ) * 60;

	if( tail )
	    secs += toInt( tail );
    }
    return secs;
}


bool uiMMBatchJobDispatcher::isPaused() const
{
    const char* txt = jrppolselfld_->text();
    bool dopause = *txt == 'P';
    if ( *txt == 'G' )
    {
	const int t = getSecs( Time::getDateTimeString() );
	const int t0 = getSecs( jrpstartfld_->text() );
	const int t1 = getSecs( jrpstopfld_->text() );

	bool run = t1 >= t0 ? t >= t0 && t <= t1
			    : t >= t0 || t <= t1;
        dopause = !run;
    }
    return dopause;
}


bool uiMMBatchJobDispatcher::rejectOK( CallBacker* )
{
    return true;
}


bool uiMMBatchJobDispatcher::acceptOK(CallBacker*)
{
    return true;
}
