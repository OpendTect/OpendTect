/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.42 2003-02-26 14:38:58 arend Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "seismmjobman.h"
#include "uilabel.h"
#include "uilistbox.h"
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
#include "executor.h"
#include "ptrman.h"
#include "lic.h"
#include <stdlib.h>

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
	, logvwer(0)
    	, iopl(*new IOParList(iopl))
    	, nrcyclesdone(0)
    	, tmpstordirfld(0)
    	, stopbut(0)
{
    const IOPar& iopar = *iopl[0];
    const char* res = iopar.find( "Target value" );
    BufferString txt( "Multi-Machine Processing " );
    if ( res && *res )
	{ txt += "'"; txt += res; txt += "' "; }
    setTitleText( txt );

    if ( LM().check(Licenser::VolOut) != getPID() )
    {
	new uiLabel( this, LM().errMsg() );
	setOkText( "Dismiss" ); setCancelText( "" );
	return;
    }

    statusBar()->addMsgFld( "Message", uiStatusBar::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", uiStatusBar::Right, 20 );
    statusBar()->addMsgFld( "NrDone", uiStatusBar::Left, 10 );
    statusBar()->addMsgFld( "Activity", uiStatusBar::Left, 1 );
    tim.tick.notify( mCB(this,uiSeisMMProc,doCycle) );

    bool isrestart = false;
    res = iopar.find( sTmpSeisID );
    if ( res )
    {
	PtrMan<IOObj> ioobj = IOM().get( MultiID(res) );
	mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())
	if ( !iostrm )
	    isrestart = ioobj ? ioobj->implExists(true) : false;
	else
	{
	    BufferString dir = File_getPathOnly(iostrm->fileName());
	    isrestart = File_exists( dir );
	}
    }

    uiSeparator* sep = 0;
    uiObject* sepattach = 0;
    bool attaligned = true;
    if ( isrestart )
    {
	jm->setRestartID( res );
	finaliseDone.notify( mCB(this,uiSeisMMProc,doCycle) );
	res = iopar.find( sTmpStorKey );
	if ( res )
	{
	    BufferString msg( sTmpStorKey ); msg += ": "; msg += res;
	    sepattach = new uiLabel( this, msg );
	    attaligned = false;
	}
    }
    else
    {
	tmpstordirfld = new uiIOFileSelect( this, sTmpStorKey, false,
					    jm->tempStorageDir() );
	tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );
	res = iopar.find( sTmpStorKey );
	if ( res && File_isDirectory(res) ) tmpstordirfld->setInput( res );
	tmpstordirfld->selectDirectory( true );
	sepattach = tmpstordirfld->uiObj();
    }

    if ( sepattach )
    {
	sep = new uiSeparator( this, "Hor sep 1", true );
	sep->attach( stretchedBelow, sepattach );
    }
    machgrp = new uiGroup( this, "Machine handling" );

    HostDataList hdl;
    rshcomm = hdl.rshComm();
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

    addbut = new uiPushButton( machgrp, ">> Add >>" );
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );
    addbut->attach( centeredRightOf, avmachfld );

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp, "Used hosts", false,
				        uiLabeledListBox::AboveMid );
    usedmachfld->setPrefWidthInChar( 30 );

    stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    stopbut->attach( alignedBelow, usedmachfld );
    vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    usedmachgrp->attach( ensureRightOf, addbut );
    machgrp->setHAlignObj( addbut );
    if ( sep )
    {
	if ( attaligned )
	    machgrp->attach( alignedBelow, sepattach );
	machgrp->attach( ensureBelow, sep );
    }

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, machgrp );
    uiLabel* lbl = new uiLabel( this, "Progress" );
    lbl->attach( alignedBelow, sep );
    nicefld = new uiSlider( this, "Nice level" );
    nicefld->attach( ensureBelow, sep );
    nicefld->attach( rightBorder );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl.defNiceLevel() );
    uiLabel* nicelbl = new uiLabel( this, "'Nice' level (0-19)", nicefld );
    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( alignedBelow, lbl );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );

    progbar = new uiProgressBar( this, "", jm->totalNr(), 0 );
    progbar->attach( widthSameAs, progrfld );
    progbar->attach( alignedBelow, progrfld );
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
    const char* txt = jm->progressMessage();
    if ( *txt ) progrfld->append( txt );
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


void uiSeisMMProc::execFinished()
{
    if ( jmfinished )
    {
	Time_sleep( 5 );
	if ( !jm->removeTempSeis() )
	    ErrMsg( "Could not remove all temporary seismics" );
	progrfld->append( "Data transferred" );
	statusBar()->message( "Finished", 0 );
	statusBar()->message( "", 3 );
	finished = true;
	setOkText( "Quit" ); setCancelText( "Quit" );
    }
    else
    {
	const bool isrestart = !tmpstordirfld;
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
	    if ( isrestart )
	    {
		BufferString msg( "Re-starting processing for inlines:\n" );
		newjm->getToDoList( msg );
		uiMSG().message( msg );
		progrfld->append( "Ready to start remaining processing" );
	    }
	    else
	    {
		int res = askRemaining( *newjm );
		if ( res == 2 )
		    { reject(this); return; }
		else if ( res == 1 )
		{
		    start_again = false;
		    setDataTransferrer( newjm );
		}
		else
		    uiMSG().message( "Please select the hosts to perform"
				     " the remaining calculations" );
	    }
	    if ( start_again )
	    {
		delete jm; jm = newjm;
		task = newjm;
		newJM();
	    }
	}
	doCycle(0);
    }
}


void uiSeisMMProc::doCycle( CallBacker* )
{
    setNiceNess();

    const int status = task->doStep();
    switch ( status )
    {
    case 0:	execFinished();						return;
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
    }

    tim.start( delay, true );
}

#define mReturn()    { deepErase( machs ); return; }
void uiSeisMMProc::updateCurMachs()
{
    ObjectSet<BufferString> machs;
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

    if ( !running && jm->nrHostsInQueue() )
    {
	if ( tmpstordirfld )
	{
	    tmpstordirfld->setSensitive(false);
	    jm->setTempStorageDir( tmpstordirfld->getInput() );
	}
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
    return true;
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
    logvwer = new uiFileBrowser( this, fname );
    logvwer->go();
}


#include "seissingtrcproc.h"
#include <fstream>

bool uiSeisMMProc::acceptOK(CallBacker*)
{
    if ( !stopbut || finished )
	return true;
    if ( jmfinished ) // Transferring data!
	return false;

    if ( usedmachfld->box()->size() && !uiMSG().askGoOn(
	    "This will stop further processing and start data transfer",false) )
	return false;

    bool mkdump = true;
    if ( mkdump )
    {

	// Stop during operation. Create a dump
	BufferString dumpfname( GetDataDir() );
	dumpfname = File_getFullPath( dumpfname, "Proc" );
	dumpfname = File_getFullPath( dumpfname, "mmbatch_dump.txt" );
	ofstream ostrm( dumpfname );
	ostream* strm = &cerr;
	if ( ostrm.fail() )
	    cerr << "Cannot open dump file '" << dumpfname << "'" << endl;
	else
	{
	    cerr << "Writing to dump file '" << dumpfname << "'" << endl;
	    strm = &ostrm;
	}

	*strm << "Multi-machine-batch dump at " << Time_getLocalString() << endl;
	if ( !jm )
	{
	    *strm << "No Job Manager. Therefore, data transfer is busy, or "
		      "should have already finished" << endl;
	    if ( !task )
		{ *strm << "No task either. Huh?" << endl; return false; }
	    mDynamicCastGet(SeisSingleTraceProc*,stp,task)
	    if ( !stp )
		*strm << "Huh? task should really be a SeisSingleTraceProc!\n"; 
	    else
		*strm << "SeisSingleTraceProc:\n"
		       << stp->nrDone() << "/" << stp->totalNr() << endl
		       << stp->message() << endl;
	    return false;
	}

	if ( task != jm )
	    *strm << "task != jm . Why?" << endl;

	jm->dump( *strm );
    }

    execFinished();
    return false;
}
