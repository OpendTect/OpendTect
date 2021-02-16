/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          August 2001
________________________________________________________________________

-*/

#include "prog.h"

#include "uidesktopservices.h"
#include "uifileselector.h"
#include "uifont.h"
#include "uigroup.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"

#include "batchjobdispatch.h"
#include "commandlineparser.h"
#include "file.h"
#include "filepath.h"
#include "helpview.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "progressmeterimpl.h"
#include "sighndl.h"
#include "timer.h"
#include "varlenarray.h"

#include <iostream>

static int cDefDelay = 500; // milliseconds.

static const uiString sStopAndQuit()
{ return od_static_tr("sStopAndQuit","Stop process and Quit"); }
static const uiString sQuitOnly()
{return od_static_tr("sStopAndQuit","Close this window"); }


class uiProgressViewer : public uiMainWin
{ mODTextTranslationClass(uiProgressViewer)
public:

		uiProgressViewer(uiParent*,const BufferString& filenm,int,
				 int delay=cDefDelay);
		~uiProgressViewer();

    static const char*	sKeyDelay()		{ return "delay"; }
    static const char*	sKeyPID()		{ return "pid"; }
    static const char*	sKeyInputFile()		{ return "inpfile"; }

protected:

    enum ProcStatus	{ None, Running, Finished, Terminated, AbnormalEnd };

    uiTextBrowser*	txtfld_;
    uiToolBar*		tb_;
    int			quittbid_;
    int			killbid_;

    int			pid_;
    const BufferString	procnm_;
    const BufferString	filenm_;
    ProcStatus		procstatus_;
    int			delay_ = cDefDelay; //Lower may cause high CPU usage
    PtrMan<od_istream>	strm_;
    Timer		timer_;

    bool		haveProcess();

    void		initWin(CallBacker*);
    void		doWork(CallBacker*);
    void		getNewPID(CallBacker*);
    void		quitFn(CallBacker*);
    void		killFn(CallBacker*);
    void		saveFn(CallBacker*);
    void		helpFn(CallBacker*);

    void		handleProcessStatus();
    bool		closeOK();

    static bool		canTerminate();
};


#define mAddButton(fnm,txt,fn) \
    tb_->addButton( fnm, txt, mCB(this,uiProgressViewer,fn), false );

uiProgressViewer::uiProgressViewer( uiParent* p, const BufferString& fnm,
				    int pid, int delay )
    : uiMainWin(p,tr("Progress Viewer"),1)
    , timer_("Progress")
    , filenm_(fnm)
    , strm_(0)
    , pid_(pid)
    , procstatus_(None)
    , tb_(0)
    , quittbid_(0)
{
    if ( mIsUdf(pid) )
	getNewPID(0);

    if ( !mIsUdf(pid_) )
	const_cast<BufferString&>( procnm_ ) = GetProcessNameForPID( pid_ );

    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    tb_ = new uiToolBar( this, uiStrings::sToolBar() );
    quittbid_ = mAddButton( "stop", haveProcess() ? sStopAndQuit() : sQuitOnly()
			    ,quitFn );
    killbid_ = mAddButton( "cancel", tr("%1 the process")
					.arg(uiStrings::sTerminate()),
			   killFn );
    mAddButton( "save", tr("Save text to a file"), saveFn );
    mAddButton( "contexthelp", uiStrings::sHelp(), helpFn );

    txtfld_ = new uiTextBrowser( this, "Progress", mUdf(int), true, true );
    uiFont& fnt = FontList().add( "Non-prop",
	    FontData(FontData::defaultPointSize(),"Courier") );
    txtfld_->setFont( fnt );

    //Ensure we have space for 80 chars
    const int nrchars = TextStreamProgressMeter::cNrCharsPerRow()+5;
    mAllocVarLenArr( char, str, nrchars+1 );
    OD::memSet( str, ' ', nrchars );
    str[nrchars] = '\0';

    int deswidth = fnt.width( toUiString(str) );
    const int desktopwidth = uiMain::theMain().desktopSize().hNrPics();
    if ( !mIsUdf(desktopwidth) && deswidth>desktopwidth )
	deswidth = desktopwidth;

    if ( deswidth > txtfld_->defaultWidth() )
	txtfld_->setPrefWidth( deswidth );

    mAttachCB( postFinalise(), uiProgressViewer::initWin );
    mAttachCB( timer_.tick, uiProgressViewer::doWork );
    mAttachCB( txtfld_->fileReOpened, uiProgressViewer::getNewPID );
}


uiProgressViewer::~uiProgressViewer()
{
    detachAllNotifiers();
}


void uiProgressViewer::initWin( CallBacker* )
{
    handleProcessStatus();
    if ( procnm_.isEmpty() || procnm_ == "bash" || procnm_ == "cmd" )
    {
	pid_ = mUdf(int);
	getNewPID(0);
    }

    if ( !timer_.isActive() )
	timer_.start( delay_, false );
}


bool uiProgressViewer::haveProcess()
{
    if ( mIsUdf(pid_) )
	return false;

    if ( isProcessAlive(pid_) )
    {
	procstatus_ = Running;
	return true;
    }

    procstatus_ = None;

    return false;
}


void uiProgressViewer::handleProcessStatus()
{
    if ( haveProcess() )
    {
	if ( !strm_ )
	{
	    if ( !File::exists(filenm_) )
	    {
		txtfld_->setText( "Starting program" );
		return;
	    }

	    strm_ = new od_istream( filenm_.str() );
	    txtfld_->setSource( filenm_.str() );
	}

	if (strm_ && strm_->isOK())
	    txtfld_->updateFilePath();

	return;
    }

    if ( procstatus_ == None )
    {
	sleepSeconds( 1 );
	if ( haveProcess() )
	    return;

	if ( !File::exists(filenm_) )
	    procstatus_ = AbnormalEnd;
	else
	{
	    if ( !strm_ )
	    {
		strm_ = new od_istream( filenm_.str() );
		txtfld_->setSource( filenm_.str() );
	    }
	    else
		strm_->reOpen();

	    BufferString lines;
	    strm_->getAll( lines );
	    if ( lines.find(Batch::JobDispatcher::sKeyFinishMsg()) )
		procstatus_ = Finished;
	    else
		procstatus_ = AbnormalEnd;

	    strm_->close();
	}
    }

    timer_.stop();
    uiString stbmsg = tr("Processing finished successfully");
    if ( procstatus_ == Terminated )
	stbmsg = tr("Processing terminated by user" );
    if ( procstatus_ == AbnormalEnd )
	stbmsg = tr("Processing stopped because of program failure");

    statusBar()->message( stbmsg );
    tb_->setToolTip( quittbid_, sQuitOnly() );
    tb_->setSensitive( killbid_, false );
    pid_ = mUdf(int);
}


void uiProgressViewer::doWork( CallBacker* )
{
    handleProcessStatus();
    if ( procstatus_ != Running )
	return;

    statusBar()->message( tr("Running process %1 with PID %2.")
				    .arg(procnm_).arg(pid_) );
}


void uiProgressViewer::getNewPID( CallBacker* )
{
    if ( !mIsUdf(pid_) || !strm_ )
	return;

    const bool activetimer = timer_.isActive();
    if ( activetimer )
	timer_.stop();

    sleepSeconds( 1. );
    strm_->reOpen();
    BufferString line;
    bool found = false;
    while ( strm_->getLine(line) )
    {
	if ( line.find("Process ID:") )
	{
	    found = true;
	    break;
	}
    }

    strm_->close();
    if ( !found )
    {
	if ( activetimer )
	    timer_.start( delay_, false );
	return;
    }

    const SeparString sepstr( line.str(), ':' );
    const int pid = sepstr.getIValue( 1 );
    if ( pid == 0 )
    {
	if ( activetimer )
	    timer_.start( delay_, false );
	return;
    }

    pid_ = pid;
    const_cast<BufferString&>( procnm_ ) = GetProcessNameForPID( pid_ );
    if ( tb_ && quittbid_ )
    {
	tb_->setToolTip( quittbid_, sStopAndQuit() );
	tb_->setSensitive( killbid_, true );
    }

    if ( activetimer )
	timer_.start( delay_, false );
}


bool uiProgressViewer::canTerminate()
{
    return gUiMsg().askGoOn( tr("Do you want to terminate the process?") );
}


void uiProgressViewer::killFn( CallBacker* cb )
{
    if ( cb && !canTerminate() )
	return;

    SignalHandling::stopProcess( pid_ );
    pid_ = mUdf(int);
    procstatus_ = Terminated;
    handleProcessStatus();
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( closeOK() )
	uiMain::theMain().exit(0);
}


bool uiProgressViewer::closeOK()
{
    if ( haveProcess() )
    {
	if ( !canTerminate() )
	    return false;

        killFn( 0 );
    }

    return true;
}


void uiProgressViewer::helpFn( CallBacker* )
{
    HelpProvider::provideHelp( mODHelpKey(mProgressViewerHelpID) );
}


void uiProgressViewer::saveFn( CallBacker* )
{
    uiFileSelector::Setup fssu( GetProcFileName("log.txt") );
    fssu.setFormat( tr("Progress logs"), "txt" );
    fssu.setForWrite( true );
    uiFileSelector uifs( this, fssu );
    uifs.caption() = uiStrings::phrSave( uiStrings::sLogs() );
    if ( uifs.go() )
    {
	od_ostream strm( uifs.fileName() );
	if ( strm.isOK() )
	   strm << txtfld_->text() << od_endl;
    }
}


static void printBatchUsage( const char* prognm )
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << prognm;
    strm << " [LOGFILE]... [OPTION]...\n";
    strm << "Monitors a batch process from its log file and process ID.\n";
    strm << "Mandatory arguments:\n";
    strm << "\t --" << uiProgressViewer::sKeyInputFile() << "\tlogfile\n";
    strm << "\t --" << uiProgressViewer::sKeyPID() << "\t\tProcess ID\n";
    strm << "Optional arguments:\n";
    strm << "\t --" << uiProgressViewer::sKeyDelay() <<"\tDelay between";
    strm << " updates in milliseconds\n";
    strm << od_endl;
}

#undef mErrRet
#define mErrRet(act) \
{ \
    act; \
    return 1; \
}

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app;
    OD::ModDeps().ensureLoaded( "uiBase" );

    auto& clp = app.commandLineParser();
    if ( argc < 2 )
	mErrRet(printBatchUsage( clp.getExecutableName() ))

    BufferString inpfile = clp.keyedString( uiProgressViewer::sKeyInputFile() );
    int pid = clp.keyedValue<int>( uiProgressViewer::sKeyPID() );
    int delay = clp.keyedValue<int>( uiProgressViewer::sKeyDelay() );
    if ( mIsUdf(delay) )
	delay = cDefDelay;

    if ( inpfile.isEmpty() )
    {
	BufferStringSet normalargs;
	clp.getNormalArguments( normalargs );
	if ( !normalargs.isEmpty() )
	{
	    const File::Path fp( normalargs.get(0) );
	    if ( BufferString(fp.extension()) == "log" ||
		 BufferString(fp.extension()) == "txt" )
		inpfile = fp.fullPath();

	    if ( normalargs.size() > 1 )
	    {
		const int tmppid = normalargs.get(1).toInt();
		if ( !mIsUdf(tmppid) || tmppid > 0 )
		    pid = tmppid;
	    }
	}
    }

    PtrMan<uiMainWin> pv = new uiProgressViewer( 0, inpfile, pid, delay );
    app.setTopLevel( pv );
    pv->show();

    return app.exec();
}
