/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.77 2004-11-11 15:38:42 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "uiseisioobjinfo.h"
#include "seisjobexecprov.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "jobinfo.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uifilebrowser.h"
#include "uiiosel.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "hostdata.h"
#include "ioparlist.h"
#include "ioman.h"
#include "iostrm.h"
#include "timer.h"
#include "timefun.h"
#include "filegen.h"
#include "filepath.h"
#include "executor.h"
#include "ptrman.h"
#include "strmprov.h"
#include "seissingtrcproc.h"
#include <stdlib.h>
#include <iostream>



uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prnm, const IOParList& pl )
	: uiDialog(p,uiDialog::Setup("Job management","","103.2.0")
		.nrstatusflds(-1)
		.fixedsize())
	, progname(prnm)
	, hdl(*new HostDataList)
    	, iopl(*new IOParList(pl))
	, avmachfld(0), usedmachfld(0)
	, jrppolselfld(0), nicefld(0)
	, tmpstordirfld(0), logvwer(0)
	, progrfld(0) , progbar(0)
	, jrpstartfld(0), jrpstopfld(0)
    	, jobprov(0), jobrunner(0)
    	, outioobjinfo(0), isrestart(false)
	, timer(0)
{
    const IOPar& iopar = *iopl[0];
    MultiID outid = iopar.find( SeisJobExecProv::outputKey(iopar) );
    outioobjinfo = new uiSeisIOObjInfo( outid );
    if ( !outioobjinfo->isOK() )
	{ setOkText( "Quit" ); setCancelText( "" ); return; }

    const int nrhosts = hdl.size();
    const bool multihost = nrhosts > 1;
    const int maxhostdisp =  nrhosts > 7 ? 8 : nrhosts;
    const int hostnmwdth = 30;
    is2d = outioobjinfo->is2D();

    setOkText( "  Dismiss  " );
    setTitleText( multihost ? "Multi-Machine Processing"
		    : (is2d ? "Multi-line processing"
			    : "Line-split processing") );
    const char* res = iopar.find( "Target value" );
    caption = "Processing";
    if ( res && *res )
	{ caption += " '"; caption += res; caption += "'"; }
    setCaption( caption );

    statusBar()->addMsgFld( "Message", uiStatusBar::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", uiStatusBar::Right, 20 );
    statusBar()->addMsgFld( "NrDone", uiStatusBar::Left, 10 );
    statusBar()->addMsgFld( "Activity", uiStatusBar::Left, 1 );

    uiSeparator* sep = 0;
    uiObject* sepattach = 0;
    bool attaligned = true;
    if ( !is2d )
    {
	BufferString tmpstordir = iopar.find( SeisJobExecProv::sKeyTmpStor );
	isrestart = tmpstordir != "";
	if ( !isrestart )
	{
	    tmpstordir = SeisJobExecProv::getDefTempStorDir();
	    FilePath fp( tmpstordir ); fp.setFileName( 0 );
	    tmpstordir = fp.fullPath();
	}

	if ( isrestart )
	{
	    BufferString msg( SeisJobExecProv::sKeyTmpStor ); msg += ": ";
	    msg += tmpstordir;
	    sepattach = new uiLabel( this, msg );
	    attaligned = false;
	}
	else
	{
	    tmpstordirfld = new uiIOFileSelect( this,
			    SeisJobExecProv::sKeyTmpStor, false, tmpstordir );
	    tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );
	    if ( tmpstordir != "" && File_isDirectory(tmpstordir) )
		tmpstordirfld->setInput( tmpstordir );
	    tmpstordirfld->selectDirectory( true );
	    tmpstordirfld->stretchHor( true );
	    sepattach = tmpstordirfld->mainObject();
	}
    }

    if ( sepattach )
    {
	sep = new uiSeparator( this, "Hor sep 1", true );
	sep->attach( stretchedBelow, sepattach );
    }

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    if ( !multihost )
	attaligned = false;
    else
    {
	avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
					  uiLabeledListBox::AboveMid );
	for ( int idx=0; idx<hdl.size(); idx++ )
	{
	    const HostData& hd = *hdl[idx];
	    BufferString nm( hd.name() );
	    const int nraliases = hd.nrAliases();
	    for ( int idx=0; idx<nraliases; idx++ )
		{ nm += " / "; nm += hd.alias(idx); }
	    avmachfld->box()->addItem( nm );
	}

	avmachfld->setPrefWidthInChar( hostnmwdth );
	avmachfld->setPrefHeightInChar( maxhostdisp );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp,
				    multihost ? "Used hosts" : "", false,
				    uiLabeledListBox::AboveMid );
    usedmachfld->setPrefWidthInChar( hostnmwdth );
    usedmachfld->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld );
	addbut = new uiPushButton( machgrp, ">> Add >>" );
	if ( avmachfld ) addbut->attach( centeredRightOf, avmachfld );
	usedmachgrp->attach( ensureRightOf, addbut );
	machgrp->setHAlignObj( avmachfld );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "Start" );
	addbut->attach( alignedBelow, usedmachfld );
	stopbut->attach( centeredBelow, usedmachfld );
    	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );

    if ( sep )
    {
	if ( false && attaligned )
	    machgrp->attach( alignedBelow, sepattach );
	machgrp->attach( ensureBelow, sep );
    }

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld = new uiSlider( jrppolgrp, "Nice level" );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl.defNiceLevel() );
    uiLabel* nicelbl = new uiLabel( jrppolgrp, "'Nice' level (0-19)" );
    nicelbl->attach( rightOf, nicefld );
    if ( avmachfld ) nicefld->setPrefWidthInChar( hostnmwdth );

    jrppolselfld = new uiComboBox( jrppolgrp );
    jrppolselfld->addItem( "Run" );
    jrppolselfld->addItem( "Pause" );
    jrppolselfld->addItem( "Go - Only between" );
    jrppolselfld->setCurrentItem( ((int)0) );
    jrppolselfld->selectionChanged.notify( mCB(this,uiSeisMMProc,jrpSel) );
    jrppolselfld->attach( alignedBelow, nicefld );
    if ( avmachfld ) jrppolselfld->setPrefWidthInChar( hostnmwdth );
    jrpworklbl = new uiLabel( jrppolgrp, "Processes" );
    jrpworklbl->attach( rightOf, jrppolselfld );

    BufferString tm = getenv("DTECT_STOP_OFFICEHOURS")
		    ? getenv("DTECT_STOP_OFFICEHOURS") : "18:00";
    jrpstartfld = new uiGenInput( jrppolgrp, "", tm );
    jrpstartfld->attach( rightOf, jrppolselfld );

    tm  = getenv("DTECT_START_OFFICEHOURS")
	? getenv("DTECT_START_OFFICEHOURS"): "7:30";
    jrpstopfld = new uiGenInput( jrppolgrp, "and", tm );
    jrpstopfld->attach( rightOf, jrpstartfld );

    jrppolgrp->setHAlignObj( nicefld );
    jrppolgrp->attach( ensureBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, jrppolgrp );

    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( ensureBelow, sep );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );

    progbar = new uiProgressBar( this, "", 1, 0 );
    progbar->attach( widthSameAs, progrfld );
    progbar->attach( alignedBelow, progrfld );

    finaliseDone.notify( mCB(this,uiSeisMMProc,initWin) );
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete logvwer;
    delete jobprov;
    delete jobrunner;
    delete outioobjinfo;
    delete timer;

    delete &hdl;
    delete &iopl;
}


void uiSeisMMProc::initWin( CallBacker* cb )
{
    jrpSel( cb );
    if ( !avmachfld && (is2d || isrestart) )
	addPush(cb);
}


void uiSeisMMProc::setNiceNess()
{
    if ( !jobrunner ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jobrunner->setNiceNess( v );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::startWork( CallBacker* )
{
    IOPar& inpiopar = *iopl[0];
    if ( tmpstordirfld )
    {
	BufferString tmpstordir = tmpstordirfld->getInput();
	if ( !File_isWritable(tmpstordir) )
	    mErrRet("The temporary storage directory is not writable")
	tmpstordir = SeisJobExecProv::getDefTempStorDir( tmpstordir );
	inpiopar.set( SeisJobExecProv::sKeyTmpStor, tmpstordir );
    }

    jobprov = new SeisJobExecProv( progname, inpiopar );
    if ( jobprov->errMsg() && *jobprov->errMsg() )
	mErrRet(jobprov->errMsg())

    jobrunner = jobprov->getRunner();
    if ( jobprov->errMsg() && *jobprov->errMsg() )
    {
	delete jobrunner; jobrunner = 0;
	mErrRet(jobprov->errMsg())
    }

    jobrunner->setFirstPort( hdl.firstPort() );
    jobrunner->setRshComm( hdl.rshComm() );
    jobrunner->setNiceNess( hdl.defNiceLevel() );

    iopl.deepErase();
    iopl += new IOPar( jobprov->pars() );
    iopl.write();
    iopl.deepErase();

    setOkText( "Finish Now" );
    setCancelText( "Abort" );

    jobrunner->jobStarted.notify( mCB(this,uiSeisMMProc,jobStarted) );
    jobrunner->jobFailed.notify( mCB(this,uiSeisMMProc,jobFailed) );

    timer = new Timer("uiSeisMMProc timer");
    timer->tick.notify( mCB(this,uiSeisMMProc,doCycle) );
    timer->start( 100, true );
}


static int getSecs( const char* txt )
{
    if ( !txt || !*txt ) return 0;
    BufferString bs( txt );
    char* mid = strchr( bs.buf(), ':' );
    if ( mid ) *mid++ = '\0';

    int secs=-1;
    if ( mid && *mid )
    {
	char* head = mid-1;
	while ( *head != ' ' && head > bs.buf() ) head--;

	char* tail = strchr( mid, ':' );
	if ( tail ) *tail++ = '\0';

	secs = atoi( head ) * 3600;
	secs += atoi( mid ) * 60;

	if( tail )
	    secs += atoi( tail );
    }   
    return secs;
}


void uiSeisMMProc::doCycle( CallBacker* )
{
    nrcyclesdone++;

    jobrunner->nextStep();
    if ( jobrunner->jobsLeft() == 0 )
    {
	if ( wrapUp(false) )
	    return;
	delete jobrunner;
	jobrunner = jobprov->getRunner();
	if ( !jobrunner )
	    return;
    }

    setNiceNess();
    updateCurMachs();
    updateAliveDisp();

    timer->start( 250, true );
}


void uiSeisMMProc::updateAliveDisp()
{
    static const int nrdispstrs = 6;
    static const char* dispstrs[]
	= { ">..", ".>.", "..>", "..<", ".<.", "<.." };
    statusBar()->message( dispstrs[ nrcyclesdone % nrdispstrs ], 3 );

    const int totsteps = jobrunner->totalNr();
    const int nrdone = jobrunner->nrDone();
    const bool hastot = totsteps > 0;
    progbar->display( hastot );
    if ( hastot )
    {
	progbar->setTotalSteps( totsteps );
	progbar->setProgress( nrdone );

	const float fpct = 100. * ((float)nrdone) / totsteps;
	int pct = (int)fpct; if ( pct > 100 ) pct = 100;
	BufferString newcap( "[" ); newcap += pct; newcap += "%] ";
	newcap += caption;
	setCaption( newcap );
    }
}


#define mReturn()    { deepErase( machs ); return; }
void uiSeisMMProc::updateCurMachs()
{
    BufferStringSet machs;
    jobrunner->showMachStatus( machs );
    sort( machs );

    const int oldsz = usedmachfld->box()->size(); 
    const int newsz = machs.size();

    int curit = oldsz ? usedmachfld->box()->currentItem() : -1;
    usedmachfld->box()->empty();
    if ( newsz )
    {
	usedmachfld->box()->addItems( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    else
	usedmachfld->box()->clear();

    mReturn();
}


int uiSeisMMProc::runnerHostIdx( const char* mach ) const
{
    if ( !jobrunner || !mach || !*mach ) return -1;

    const ObjectSet<JobHostInfo>& hi = jobrunner->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(mach) )
	    return idx;
    }
    return -1;
}


void uiSeisMMProc::addPush( CallBacker* )
{
    uiListBox* lb = avmachfld ? avmachfld->box() : 0;
    const int nrmach = lb ? lb->size() : 1;
    if ( nrmach < 1 ) return;
    const int nrsel = lb ? lb->nrSelected() : 1;
    if ( nrsel < 1 )
	mErrRet("Please select one or more hosts")

    for ( int idx=0; idx<nrmach; idx++ )
    {
	if ( lb && !lb->isSelected(idx) ) continue;

	if ( !jobrunner )
	    startWork(0);
	if ( !jobrunner )
	    continue;

	BufferString hnm = lb ? lb->textOfItem( idx ) : hdl[0]->name();
	char* ptr = strchr( hnm.buf(), ' ' );
	if ( ptr ) *ptr = '\0';

	const HostData* hd = hdl.find( hnm.buf() );
	if ( !hd ) { pErrMsg("Huh"); continue; }
	if ( !jobrunner->addHost(*hd) && jobrunner->jobsLeft() > 0 )
	{
	    BufferString msg = "Could not start job";
	    if ( lb )
		{ msg += " on "; msg += hnm; }
	    uiMSG().warning( msg );
	}
    }
}

const char* uiSeisMMProc::curUsedMachName()
{
   static BufferString mach;
   mach = usedmachfld->box()->getText();

   char* ptr = strstr( mach.buf(), " -:- ");
   if ( ptr ) *ptr='\0';

   return mach;
}

void uiSeisMMProc::stopPush( CallBacker* )
{
    int rhidx = runnerHostIdx( curUsedMachName() );
    if ( rhidx >= 0 )
	jobrunner->removeHost( rhidx );
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
    BufferString hostnm( curUsedMachName() );
    JobHostInfo* jhi = 0;
    const ObjectSet<JobHostInfo>& hi = jobrunner->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(hostnm) )
	    { jhi = hi[idx]; break; }
    }
    if ( !jhi ) return;

    JobInfo* ji = jobrunner->currentJob( jhi );
    FilePath logfp( jobrunner->getBaseFilePath(*ji, jhi->hostdata_) );
    logfp.setExtension( ".log", false );

    delete logvwer;
    logvwer = new uiFileBrowser( this, uiFileBrowser::Setup(logfp.fullPath())
					.scroll2bottom(true) );
    logvwer->go();
}


static void addObjNm( BufferString& msg, const JobRunner* jr, int nr )
{
    msg += jr->descProv()->objType(); msg += " ";
    msg += jr->descProv()->objName( nr );
}


void uiSeisMMProc::jobStarted( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->notifyJob();
    BufferString msg( "Started processing " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }
    progrfld->append( msg );
}


void uiSeisMMProc::jobFailed( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->notifyJob();
    BufferString msg( "Failure for " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }
    if ( ji.curmsg_ != "" )
	{ msg += ": "; msg += ji.curmsg_; }
    progrfld->append( msg );
}


void uiSeisMMProc::jrpSel( CallBacker* )
{
    const bool isgo = *jrppolselfld->text() == 'G';
    jrpstartfld->display( isgo );
    jrpstopfld->display( isgo );
    jrpworklbl->display( !isgo );
}


static void rmTmpSeis( SeisJobExecProv* jp )
{
    Time_sleep( 2.25 );
    if ( !jp->removeTempSeis() )
	ErrMsg( "Could not remove all temporary seismics" );
}


bool uiSeisMMProc::readyForPostProcess()
{
    if ( jobrunner->jobsLeft() > 0 ) return false;
    const int nrfailed = jobrunner->nrJobs(true);
    if ( nrfailed < 1 ) return true;

    BufferString msg( "Failed " );
    msg += jobrunner->descProv()->objType();
    msg += nrfailed > 1 ? "s:\n" : ": ";
    BufferString newpart;
    bool needspace = false;
    for ( int idx=0; idx<nrfailed; idx++ )
    {
	const JobInfo& ji = jobrunner->jobInfo( idx, true );
	if ( needspace ) newpart += " ";
	newpart += jobrunner->descProv()->objName( ji.descnr_ );
	if ( newpart.size() < 70 )
	    needspace = true;
	else
	{
	    msg += newpart;
	    newpart = "\n"; needspace = false;
	}
    }
    msg += newpart;
    msg += "\n\nDo you want to re-try?";
    return !uiMSG().askGoOn(msg);
}


bool uiSeisMMProc::wrapUp( bool force )
{
    if ( !force && !readyForPostProcess() )
	return false;

    Executor* exec = jobprov ? jobprov->getPostProcessor() : 0;
    if ( !exec ) return true;

    uiExecutor uiex( this, *exec );
    if ( !uiex.go() )
	{ delete exec; return false; }
    delete exec;

    setOkText( "Dismiss" );
    setCancelText( "Dismiss" );
    rmTmpSeis( jobprov );
    return true;
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    if ( !outioobjinfo->ioObj() ) return true;

    int res = 0;
    if ( jobrunner->jobsInProgress() > 0 )
    {
	BufferString msg = "This will stop all processing!\n\n";
	msg += "Do you want to remove already processed data?";
	res = uiMSG().askGoOnAfter( msg );
    }
    if ( res == 2 ) return false;

    jobrunner->stopAll();
    if ( res == 0 )
	rmTmpSeis( jobprov );

    return true;
}


bool uiSeisMMProc::acceptOK(CallBacker*)
{
    if ( !outioobjinfo->ioObj() ) return true;

    if ( usedmachfld->box()->size() && !uiMSG().askGoOn(
	    "This will stop further processing and wrap up",false) )
	return false;
    
    jobrunner->stopAll();
    return wrapUp( true );
}
