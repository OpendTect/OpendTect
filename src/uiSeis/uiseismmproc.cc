/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.68 2004-10-21 13:17:52 dgb Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "seismmjobman.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uifilebrowser.h"
#include "uiiosel.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimain.h"
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

const char* sTmpStorKey = "Temporary storage directory";
const char* sTmpSeisID = "Temporary seismics";


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm,
			    const IOParList& iopl )
	: uiDialog(p,uiDialog::Setup(
		    getFirstJM(prognm,*iopl[0]).name(),
		    "","103.2.0")
		.nrstatusflds(-1)
		.oktext("Finish Now")
		.canceltext("Abort")
		.fixedsize())
	, tim(*new Timer(name()))
    	, delay(500)
    	, running(false)
    	, finished(false)
    	, jmfinished(false)
    	, iopl(*new IOParList(iopl))
    	, nrcyclesdone(0)
    	, estmbs(0)
    	, targetioobj(0)
	, avmachfld(0)
	, usedmachfld(0)
	, jrppolselfld(0)
	, addbut(0)
	, stopbut(0)
	, vwlogbut(0)
	, autorembut(0)
	, detectbut(0)
	, tmpstordirfld(0)
	, progrfld(0)
	, logvwer(0)
	, machgrp(0)
	, jrpstartfld(0)
	, jrpstopfld(0)
	, nicefld(0)
	, progbar(0)
{
    const IOPar& iopar = *iopl[0];
    const char* res = iopar.find( "Target value" );
    caption = "Processing";
    if ( res && *res )
    { caption += " '"; caption += res; caption += "'"; }
    setCaption( caption );

    setTitleText( "Multi-Machine Processing" );
    res = iopar.find( "Estimated MBs" );
    if ( res ) estmbs = atoi( res );

    statusBar()->addMsgFld( "Message", uiStatusBar::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", uiStatusBar::Right, 20 );
    statusBar()->addMsgFld( "NrDone", uiStatusBar::Left, 10 );
    statusBar()->addMsgFld( "Activity", uiStatusBar::Left, 1 );
    tim.tick.notify( mCB(this,uiSeisMMProc,doCycle) );

    tmpstordir = iopar.find( sTmpStorKey );
    BufferString restartid( iopar.find( sTmpSeisID ) );
    res = iopar.find( "Output.1.Seismic ID" );
    if ( !res ) res = iopar.find( "Output.0.Seismic ID" );
    targetioobj = IOM().get( res );
    bool isrestart = false;
    if ( restartid != "" )
    {
	PtrMan<IOObj> ioobj = IOM().get( MultiID(restartid) );
	mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())
	if ( !iostrm )
	    isrestart = ioobj ? ioobj->implExists(true) : false;
	else
	{
	    tmpstordir = FilePath(iostrm->fileName()).pathOnly();
	    isrestart = File_exists( tmpstordir );
	}
    }

    uiSeparator* sep = 0;
    uiObject* sepattach = 0;
    bool attaligned = true;
    if ( isrestart )
    {
	jm->setRestartID( restartid );
	BufferString msg( sTmpStorKey ); msg += ": "; msg += tmpstordir;
	sepattach = new uiLabel( this, msg );
	attaligned = false;
    }
    else
    {
	tmpstordir = jm->tempStorageDir();
	tmpstordirfld = new uiIOFileSelect( this, sTmpStorKey, false,
					    tmpstordir );
	tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );
	if ( tmpstordir != "" && File_isDirectory(tmpstordir) )
	    tmpstordirfld->setInput( tmpstordir );
	tmpstordirfld->selectDirectory( true );
	sepattach = tmpstordirfld->mainObject();
    }

    if ( sepattach )
    {
	sep = new uiSeparator( this, "Hor sep 1", true );
	sep->attach( stretchedBelow, sepattach );
    }
    machgrp = new uiGroup( this, "Machine handling" );

    HostDataList hdl;
    rshcomm = hdl.rshComm();

    const bool multihost = hdl.size() > 1;
    if ( multihost )
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

	avmachfld->setPrefWidthInChar( 30 );
	machgrp->setHAlignObj( avmachfld );

    }
    else
	attaligned = false;


    uiGroup* usedmachgrp = new uiGroup( machgrp, "Machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp,
				    multihost ? "Used hosts" : "", false,
				    uiLabeledListBox::AboveMid );
    usedmachfld->setPrefWidthInChar( 30 );


    stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    if( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld );

	addbut = new uiPushButton( machgrp, ">> Add >>" );

	if ( avmachfld ) addbut->attach( centeredRightOf, avmachfld );

	usedmachgrp->attach( ensureRightOf, addbut );
	machgrp->setHAlignObj( addbut );

    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "Start" );
	addbut->attach( alignedBelow, usedmachfld );
	stopbut->attach( rightOf, addbut );

    	machgrp->setHAlignObj( stopbut );
    }

    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );


    if ( sep )
    {
	if ( attaligned )
	    machgrp->attach( alignedBelow, sepattach );
	machgrp->attach( ensureBelow, sep );
    }

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );
    jrppolselfld = new uiLabeledComboBox( jrppolgrp, "Assignment" );
    jrppolselfld->box()->addItem( "Work" );
//    jrppolselfld->box()->addItem( "Finish current jobs" );
    jrppolselfld->box()->addItem( "Pause" );
    jrppolselfld->box()->addItem( "Go - Only between" );
    jrppolselfld->box()->setCurrentItem( ((int)0) );

    jrpstarttime = getenv("DTECT_STOP_OFFICEHOURS")
		 ? getenv("DTECT_STOP_OFFICEHOURS") : "18:00";
    jrpstoptime  = getenv("DTECT_START_OFFICEHOURS")
		 ? getenv("DTECT_START_OFFICEHOURS"): "7:30";
 
    jrpstartfld = new uiGenInput( jrppolgrp, "", jrpstarttime );
    jrpstartfld->attach( rightOf, jrppolselfld );
    jrpstopfld = new uiGenInput( jrppolgrp, "and", jrpstoptime );
    jrpstopfld->attach( rightOf, jrpstartfld );

    jrpstartfld->valuechanged.notify( mCB(this,uiSeisMMProc,startStopUpd) );
    jrpstopfld->valuechanged.notify( mCB(this,uiSeisMMProc,startStopUpd) );

    jrppolselfld->box()->selectionChanged.notify(
	    mCB(this,uiSeisMMProc,jrpSel) );


    finaliseDone.notify( mCB(this,uiSeisMMProc,jrpSel) );

    if ( multihost )	jrppolgrp->setHAlignObj( jrpstartfld );
    else		jrppolgrp->setHAlignObj( jrppolselfld );

    jrppolgrp->attach( alignedBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, jrppolgrp );
    autorembut = new uiCheckBox( this, "Auto-fill" );
    autorembut->attach( alignedBelow, sep );

#ifdef DETECT_HUMAN_ACTIVITY
    detectbut = new uiCheckBox( this, "Detect human activity" );
    detectbut->attach( ensureBelow, sep );
    detectbut->attach( rightBorder );
#else
    detectbut = 0;
#endif

    nicefld = new uiSlider( this, "Nice level" );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl.defNiceLevel() );

    if ( detectbut )
	nicefld->attach( leftOf, detectbut );
    else
    {
	nicefld->attach( ensureBelow, sep );
	nicefld->attach( rightBorder );
    }

    uiLabel* nicelbl = new uiLabel( this, "'Nice' level (0-19)", nicefld );
    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( alignedBelow, autorembut );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );

    progbar = new uiProgressBar( this, "", jm->totalNr(), 0 );
    progbar->attach( widthSameAs, progrfld );
    progbar->attach( alignedBelow, progrfld );

    if ( isrestart )
	finaliseDone.notify( mCB(this,uiSeisMMProc,doCycle) );
}


Executor& uiSeisMMProc::getFirstJM( const char* prognm, const IOPar& iopar )
{
    const char* res = iopar.find( "Output Seismics Key" );
    BufferString seisoutkey( res ? res : "Output.1.Seismic ID" );
    res = iopar.find( "Inline Range Key" );
    BufferString ilrgkey( res ? res : "Output.1.In-line range" );
    task = jm = new SeisMMJobMan( prognm, iopar, seisoutkey, ilrgkey );
    newJM();
    return *jm;
}


uiSeisMMProc::~uiSeisMMProc()
{
    if ( !stopbut ) return;

    delete logvwer;
    delete jm;
    if ( task != jm ) delete task;
    delete &tim;
}


void uiSeisMMProc::newJM()
{
    if ( !jm ) return;
    jm->poststep.notify( mCB(this,uiSeisMMProc,postStep) );
}


void uiSeisMMProc::postStep( CallBacker* )
{
    BufferString pm;
    jm->fetchProgressMessage( pm );

    if ( pm.size() ) progrfld->append( pm );
    updateCurMachs();
}


void uiSeisMMProc::setNiceNess()
{
    if ( !jm ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jm->setNiceNess( v );
}


void uiSeisMMProc::setDataTransferrer( SeisMMJobMan* newjm )
{
    delete newjm; newjm = 0;
    jmfinished = true;
    task = jm->dataTransferrer();
    delay = 0;
    progrfld->append( "Starting data transfer" );
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


bool uiSeisMMProc::pauseJobs() const
{
    const char* txt = jrppolselfld->box()->text();
    if ( *txt == 'W' || *txt == 'F' ) return false; // Work / Finish current job
    if ( *txt == 'P' ) return true;		    // Pause

    // Only between

    const int t = getSecs( Time_getLocalString() );
    const int t0 = getSecs( jrpstarttime );
    const int t1 = getSecs( jrpstoptime );

    bool run = t1 >= t0 ? t >= t0 && t <= t1
			: t >= t0 || t <= t1;
    return !run;
}


static int askRemaining( const SeisMMJobMan& jm )
{
    const int nrlines = jm.totalNr();
    if ( nrlines < 1 ) return 1;

    int linenr = jm.lineToDo(0);
    BufferString msg;
    if ( nrlines == 1 )
    {
	msg = "Inline "; msg += linenr;
	msg += " was not calculated.\n"
		"This may be due to a gap or an unexpected error.\n"
		"Do you want to re-try this line?";
    }
    else
    {
	msg = "The following inlines were not calculated.\n"
		" (this may be due to gaps or unexpected errors):\n";
	jm.getToDoList( msg );
	msg += "\nDo you want to try to calculate these lines?";
    }

    return uiMSG().askGoOnAfter( msg, "Quit program" );
}


void uiSeisMMProc::execFinished( bool userestart )
{
    if ( jmfinished )
    {
	Time_sleep( 3 );
	if ( !jm->removeTempSeis() )
	    ErrMsg( "Could not remove all temporary seismics" );
	progrfld->append( "Data transferred" );
	statusBar()->message( "Finished", 0 );
	statusBar()->message( "", 3 );
	finished = true;
	setOkText( "Quit" ); setCancelText( "Quit" );
	if ( targetioobj )
	    uiSeisIOObjInfo(*targetioobj,false).provideUserInfo();
    }
    else
    {
	const bool isrestart = userestart && !tmpstordirfld;
	stopRunningJobs();
	updateCurMachs();
	progrfld->append( "Checking integrity of processed data" );
	uiMain::processEvents( 100 );
	SeisMMJobMan* newjm = new SeisMMJobMan( *jm );
	if ( newjm->totalNr() < 1 )
	    setDataTransferrer( newjm );
	else
	{
	    bool start_again = true;
	    bool add_localhost = false;
	    if ( isrestart )
	    {
		BufferString msg( "Re-starting processing for inlines:\n" );
		newjm->getToDoList( msg );
		uiMSG().message( msg );
		progrfld->append( "Ready to start remaining processing" );
	    }
	    else
	    {
		const bool autorem = autoRemaining();
		int res = autorem ? 0 : askRemaining( *newjm );
		if ( res == 2 )
		    { reject(this); return; }
		else if ( res == 1 )
		{
		    start_again = false;
		    setDataTransferrer( newjm );
		}
		else
		{
		    if ( !autorem )
			uiMSG().message( "Please select the hosts to perform"
					 " the remaining calculations" );
		    else
			add_localhost = true;
		}
	    }
	    if ( start_again )
	    {
		delete jm; jm = newjm;
		if ( add_localhost )
		{
		    if ( avmachfld )
		    {
			avmachfld->box()->selAll( false );
			avmachfld->box()->setSelected( 0, true );
		    }
		    addPush(0);
		}
		task = newjm;
		newJM();
	    }
	}
	doCycle(0);
    }
}


bool uiSeisMMProc::autoRemaining() const
{
    return autorembut && autorembut->isChecked();
}


void uiSeisMMProc::doCycle( CallBacker* )
{
    setNiceNess();
    const bool pause = pauseJobs();
    if ( jm ) jm->setPause( pause );

    const int status = task->doStep();
    switch ( status )
    {
    case 0:	execFinished(true);					return;
    case -1:	uiMSG().error( task->message() );	done( -1 );	return;
    case 2:	uiMSG().warning( task->message() );			break;
    }

    nrcyclesdone++;
    if ( nrcyclesdone == 1 && task == jm )
    {
	iopl[0]->set( sTmpStorKey, jm->tempStorageDir() );
	iopl[0]->set( sTmpSeisID, jm->tempSeisID() );
	iopl.write();
    }

    prepareNextCycle( delay );
}


void uiSeisMMProc::prepareNextCycle( int msecs )
{
    uiStatusBar& sb = *statusBar();
    sb.message( task->message(), 0 );
    sb.message( task->nrDoneText(), 1 );
    const int nrdone = task->nrDone();
    BufferString str; str += nrdone;
    sb.message( str, 2 );
    static const int nrdispstrs = 4;
    static const char* dispstrs[]
	= { "o..", ".o.", "..o", ".o." };
    if ( task == jm )
	sb.message( dispstrs[ nrcyclesdone % nrdispstrs ], 3 );
    else
	sb.message("");

    const int totsteps = task->totalNr();
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

    tim.start( delay, true );
}

#define mReturn()    { deepErase( machs ); return; }
void uiSeisMMProc::updateCurMachs()
{
    BufferStringSet machs;
    jm->getActiveMachines( machs );
    sort( machs );
    const int oldsz = usedmachfld->box()->size();

    const int newsz = machs.size();
    bool chgd = newsz != oldsz;
    if ( !chgd )
    {
	// Check in detail
	for ( int idx=0; idx<oldsz; idx++ )
	    if ( *machs[idx] != usedmachfld->box()->textOfItem(idx) )
		{ chgd = true; break; }
    }

    if ( !chgd ) mReturn();

    int curit = oldsz ? usedmachfld->box()->currentItem() : -1;
    usedmachfld->box()->empty();
    if ( newsz )
    {
	usedmachfld->box()->addItems( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    stopbut->setSensitive( newsz );
    vwlogbut->setSensitive( newsz );

    mReturn();
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    if ( !stopbut || !running ) return true;

    BufferString msg;
    int res = 0;
    if ( !finished )
    {
	msg = "This will stop all processing!\n\n";
	msg += "Do you want to remove already processed data?";
	res = uiMSG().askGoOnAfter( msg );
    }

    if ( res == 2 )
	return false;

    stopRunningJobs();

    if ( res == 0 )
    {
	Time_sleep( 2.25 );

	if ( !jm->removeTempSeis() )
	    ErrMsg( "Could not remove all temporary seismics" );
	jm->cleanup();
    }

    return true;
}


void uiSeisMMProc::stopRunningJobs()
{
    const int nrleft = usedmachfld->box()->size();
    if ( nrleft )
    {
	statusBar()->message( "Stopping running jobs" );
	for ( int idx=0; idx<nrleft; idx++ )
	{
	    usedmachfld->box()->setCurrentItem(0);
	    stopPush( 0 );
	}
	statusBar()->message( "" );
    }
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::addPush( CallBacker* )
{
    if ( !running )
    {
	if ( tmpstordirfld )
	{
	    tmpstordir = tmpstordirfld->getInput();
	    if ( !File_exists(tmpstordir) )
		mErrRet("Please enter a valid temporary storage directory")
	    else if ( !File_isDirectory(tmpstordir) )
	    {
		tmpstordir = FilePath(tmpstordir).pathOnly();
		tmpstordirfld->setInputText( tmpstordir );
	    }
	}

	if ( !File_isWritable(tmpstordir) )
	    mErrRet("The temporary storage directory is not writable")

	if ( estmbs > 0 )
	{
	    const int tempstoravail = File_getFreeMBytes( tmpstordir );
	    const int targetavail = targetioobj ? GetFreeMBOnDisk(targetioobj)
						: tempstoravail;
	    const bool samedisk = tempstoravail == targetavail;
	    int needed = estmbs;
	    if ( samedisk )
		needed += estmbs;
	    if ( needed > tempstoravail )
	    {
		BufferString msg( samedisk ? "The target" : "The temporary" );
		msg += " disk may not have enough free disk space.";
		msg += "\nNeeded (estimate): "; msg += needed; msg += " MB";
		msg += "\nAvailable: "; msg += tempstoravail; msg += " MB";
		msg += "\nDo you wish to continue?";
		if ( !uiMSG().askGoOn( msg ) )
		    return;
	    }
	}
    }

    if ( avmachfld )
    {
	for( int idx=0; idx<avmachfld->box()->size(); idx++ )
	{
	    if ( avmachfld->box()->isSelected(idx) )
	    {
		BufferString hnm( avmachfld->box()->textOfItem(idx) );
		char* ptr = strchr( hnm.buf(), '/' );
		if ( ptr ) *(--ptr) = '\0';
		jm->addHost( hnm );
	    }
	}
    }
    else jm->addHost( "localhost" );

    if ( !running && jm->nrHostsInQueue() )
    {
	if ( tmpstordirfld )
	    tmpstordirfld->setSensitive(false);
	jm->setTempStorageDir( tmpstordir );
	jm->setRemExec( rshcomm );
	running = true;
	prepareNextCycle(0);
    }
}


bool uiSeisMMProc::getCurMach( BufferString& mach ) const
{
    int curit = usedmachfld->box()->currentItem();
    if ( curit < 0 ) return false;

    mach = usedmachfld->box()->textOfItem(curit);

    char* ptr = strchr( mach.buf(), ':' );
    if ( ptr ) *ptr = '\0';

    return mach.size() > 0;
}


void uiSeisMMProc::stopPush( CallBacker* )
{
    BufferString mach;
    if ( !getCurMach(mach) ) { pErrMsg("Can't find machine"); return; }

    jm->removeHost( mach );

    updateCurMachs();
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
    BufferString mach;
    if ( !getCurMach(mach) ) return;

    BufferString fname;
    if ( !jm->getLogFileName(mach,fname) )
	mErrRet("Cannot find log file")

    delete logvwer;
    logvwer = new uiFileBrowser( this, uiFileBrowser::Setup(fname)
					.scroll2bottom(true) );
    logvwer->go();
}


void uiSeisMMProc::jrpSel( CallBacker* )
{
    const char* txt = jrppolselfld->box()->text();
    const bool doshw = *txt == 'G';
    jrpstartfld->display( doshw );
    jrpstopfld->display( doshw );
}


void uiSeisMMProc::startStopUpd( CallBacker* )
{
    jrpstarttime = jrpstartfld->text();
    jrpstoptime = jrpstopfld->text();
}


bool uiSeisMMProc::acceptOK(CallBacker*)
{
    if ( !stopbut || finished )
	return true;
    if ( jmfinished ) // Transferring data!
	return false;

    if ( usedmachfld->box()->size() && !uiMSG().askGoOn(
	    "This will stop further processing and start data transfer",false) )
	return false;

    bool mkdump = getenv("DTECT_MMBATCH_DUMP");
    if ( mkdump )
    {
	// Stop during operation. Create a dump
	FilePath fp( GetDataDir() );
	fp.add( "Proc" ).add( "mmbatch_dump.txt" );
	BufferString dumpfname = fp.fullPath();
	StreamData sd = StreamProvider( dumpfname ).makeOStream();
	std::ostream* strm = &std::cerr;
	if ( !sd.usable() )
	    std::cerr << "Cannot open dump file '" << dumpfname
		      << "'" << std::endl;
	else
	{
	    std::cerr << "Writing to dump file '" << dumpfname
		      << "'" << std::endl;
	    strm = sd.ostrm;
	}

	*strm << "Multi-machine-batch dump at "
	      << Time_getLocalString() << std::endl;
	if ( !jm )
	{
	    *strm << "No Job Manager. Therefore, data transfer is busy, or "
		      "should have already finished" << std::endl;
	    if ( !task )
		{ *strm << "No task either. Huh?" << std::endl; return false; }
	    mDynamicCastGet(SeisSingleTraceProc*,stp,task)
	    if ( !stp )
		*strm << "Huh? task should really be a SeisSingleTraceProc!\n"; 
	    else
		*strm << "SeisSingleTraceProc:\n"
		       << stp->nrDone() << "/" << stp->totalNr() << std::endl
		       << stp->message() << std::endl;
	    return false;
	}

	if ( task != jm )
	    *strm << "task != jm . Why?" << std::endl;

	jm->dump( *strm );

	sd.close();
    }

    execFinished( false );
    return false;
}
