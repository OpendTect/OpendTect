/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/


#include "batchprog.h"

#include "ascstream.h"
#include "batchjobdispatch.h"
#include "batchserviceservermgr.h"
#include "commandlineparser.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "ioman.h"
#include "iopar.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "netserver.h"
#include "odjson.h"
#include "oscommand.h"
#include "plugins.h"
#include "pythonaccess.h"
#include "sighndl.h"
#include "od_ostream.h"
#include "timer.h"

#ifndef __win__
# include "_execbatch.h"
#endif

#include "oddirs.h"

static const char* sKeyPrimaryHost()
{ return OS::MachineCommand::sKeyPrimaryHost(); }
static const char* sKeyPrimaryPort()
{ return OS::MachineCommand::sKeyPrimaryPort(); }
static const char* sKeyJobID()	{ return OS::MachineCommand::sKeyJobID(); }
static const char* sKeyODServer()    { return ServiceMgrBase::sKeyODServer(); }
static const char* sKeyPort()		{ return Network::Server::sKeyPort(); }
static const char* sKeyIsolationScript()
{ return OS::MachineCommand::sKeyIsolationScript(); }
static const char* sKeyActivatePath()
{ return OD::PythonAccess::sKeyActivatePath(); }

mDefineEnumUtils(BatchProgram,Status,"Batch program status")
{
    "Starting", "Parsing failed", "Parsing OK",
    "Setting job commmunication failed", "Setting job commmunication OK",
    "Opening log stream failed", "Opening log stream OK",
    "Waiting for instructions", "Work started", "Work error",
    "Work paused", "More work to do", "Work finished OK",
    "Process killed", "Lock file created", 0
};


BatchProgram* BatchProgram::inst_ = nullptr;

BatchProgram& BP()
{
    if ( !BatchProgram::inst_ )
    {
	DisableAutoSleep();
	BatchProgram::inst_ = new BatchProgram;
    }
    return *BatchProgram::inst_;
}


const char* BatchProgram::sKeyFinishMsg()
{
    return Batch::JobDispatcher::sKeyFinishMsg();
}


void BatchProgram::deleteInstance( int retcode )
{
    delete BatchProgram::inst_;
    IOM().applClosing();
    EnableAutoSleep();
    ApplicationData::exit( retcode );
}


BatchProgram::BatchProgram()
    : NamedCallBacker("Batch Program")
    , iopar_(new IOPar)
    , timer_(new Timer("Starting"))
    , eventLoopStarted(this)
    , startDoWork(this)
    , pause(this)
    , resume(this)
    , killed(this)
    , endWork(this)
{
    mAttachCB( timer_->tick, BatchProgram::eventLoopStartedCB );
    timer_->start(0, true);
    //Single shot. Will only be hit after the event loop is started.
}


BatchProgram::~BatchProgram()
{
    delete timer_;
    File::remove( getLockFileFP() );
    if ( strmismine_ )
	delete strm_;
    else
	strm_->close();

    deleteAndZeroPtr( clparser_ );
    delete iopar_;
    delete comm_;
    deepErase( requests_ );
}


const BufferString BatchProgram::getLockFileFP() const
{
    FilePath fp( name() );
    fp.setExtension( "lock" );
    return fp.fullPath();
}

bool BatchProgram::isOK() const
{
    return status_ != ParseFail && status_ != CommFail &&
	   status_ != LogFail && status_ != WorkFail && status_ != Killed;
}


bool BatchProgram::updateLockFilePars() const
{
    IOPar lockpars;
    lockpars.add( "System ID", GetLocalHostName() );
    lockpars.add( "User ID", GetUserNm() );
    lockpars.add( "Process ID", GetPID() );
    lockpars.write( getLockFileFP(), nullptr );
    return true;
}


bool BatchProgram::init()
{
    Threads::Locker lckr( statelock_ );
    status_ = parseArguments() ? ParseOK : ParseFail;
    if ( status_ == ParseOK )
	status_ = initComm() ? CommOK : CommFail;
    if ( status_ == CommOK )
	status_ = initLogging() ? LogOK : LogFail;

    if ( status_ == LogOK && updateLockFilePars() )
	status_ = LockOK;

    return status_ == LockOK;
}


bool BatchProgram::parseArguments()
{
    delete clparser_;

    OD::ModDeps().ensureLoaded( "Batch" );

#   define mGetKeyedVal(ky,val) \
	clparser_->setKeyHasValue( ky() ); \
	clparser_->getVal( ky(), val )

    clparser_ = new CommandLineParser;
    clparser_->setKeyHasValue( sKeyDataDir() );
    clparser_->setKeyHasValue( sKeySurveyDir() );
    clparser_->setKeyHasValue( OS::CommandExecPars::sKeyPriority() );
    clparser_->setKeyHasValue( sKeyODServer() );
    clparser_->setKeyHasValue( sKeyPort() );
    clparser_->setKeyHasValue( sKeyIsolationScript() );
    clparser_->setKeyHasValue( sKeyActivatePath() );

    BufferString primaryhost;
    int primaryport = -1;
    mGetKeyedVal( sKeyPrimaryHost, primaryhost );
    mGetKeyedVal( sKeyPrimaryPort, primaryport );
    mGetKeyedVal( sKeyJobID, jobid_ );
    if ( primaryhost.size() && primaryport > 0 )  // both must be set.
	comm_ = new JobCommunic( primaryhost, primaryport, jobid_ );

    BufferString isolatescriptfnm, pythonactivatepath;
    if ( clparser_->getVal(sKeyIsolationScript(),isolatescriptfnm) &&
	 File::exists(isolatescriptfnm.buf()) )
	OS::MachineCommand::setIsolationScript( isolatescriptfnm );

    if ( clparser_->getVal(sKeyActivatePath(),pythonactivatepath) &&
	 File::exists(pythonactivatepath.buf()) )
	OD::PythonAccess::setPythonActivator( pythonactivatepath );

    BufferString parfilnm;
    BufferStringSet normalargs;
    clparser_->getNormalArguments( normalargs );
    for ( int idx=normalargs.size()-1; idx>=0; idx-- )
    {
	const FilePath parfp( normalargs.get(idx) );

	parfilnm = parfp.fullPath();
	parfilnm.replace( '%', ' ' );
	if ( File::exists(parfilnm) )
	    break;

	parfilnm.setEmpty();
    }

    const bool simplebatch = clparser_->hasKey( sKeySimpleBatch() );
    if ( parfilnm.isEmpty() && !simplebatch )
    {
	errorMsg( tr("%1: No existing parameter file name specified")
		 .arg(clparser_->getExecutableName()) );
	return false;
    }

    setName( parfilnm );
    if ( !parfilnm.isEmpty() )
    {
	od_istream odstrm( parfilnm );
	if ( !odstrm.isOK() )
	{
	    errorMsg( tr("%1: Cannot open parameter file: %2")
			.arg( clparser_->getExecutableName() )
			.arg( parfilnm ));
	    return false;
	}

	ascistream aistrm( odstrm, true );
	if ( sKey::Pars() != aistrm.fileType() )
	{
	    errorMsg( tr("%1: Input file %2 is not a parameter file")
			.arg( clparser_->getExecutableName() )
			.arg( parfilnm ));
	    od_cerr() << aistrm.fileType() << od_endl;
	    return false;
	}

	iopar_->getFrom( aistrm );
	odstrm.close();
    }

    if ( iopar_->size() == 0 && !simplebatch )
    {
	errorMsg( tr("%1: Invalid input file %2")
		    .arg( clparser_->getExecutableName() )
		    .arg( parfilnm) );
	return false;
    }

    BufferString res = iopar_->find( sKey::LogFile() );
    if ( !res )
	iopar_->set( sKey::LogFile(), od_stream::sStdIO() );

#define mSetDataRootVar(str) \
	SetEnvVar( __iswin__ ? "DTECT_WINDATA" : "DTECT_DATA", str );

    if ( clparser_->getVal(sKeyDataDir(),res) && File::isDirectory(res) )
    {
	mSetDataRootVar( res );
	iopar_->set( sKey::DataRoot(), res );
    }
    else if ( iopar_->get(sKey::DataRoot(),res) && File::isDirectory(res) )
	mSetDataRootVar( res );

    if ( simplebatch && clparser_->getVal(sKeySurveyDir(),res) )
	iopar_->set( sKey::Survey(), res );
    else if ( !iopar_->get(sKey::Survey(),res) )
    {
	errorMsg( tr("Invalid parameter file %1\nSurvey key is missing.")
			.arg( parfilnm ) );
	return false;
    }

    if ( res.isEmpty() || !IOM().setSurvey(res) )
	{ errorMsg( tr("Cannot set the survey") ); return false; }

    killNotify( true );
    return true;
}


bool BatchProgram::initComm()
{
    if ( comm_ && !comm_->sendPID(GetPID()) )
    {
	errorMsg( tr("Could not contact primary host. Exiting."), true );
	return false;
    }

    return true;
}


bool BatchProgram::initLogging()
{
    BufferString res = pars().find( sKey::LogFile() );
    if ( res == "stdout" ) res.setEmpty();

    if ( strmismine_ )
	delete strm_;

#ifdef __cygwin__
    const bool hasviewprogress = false;
#else
    const bool hasviewprogress = true;
#endif

    if ( hasviewprogress && res && res=="window" )
    {
	OS::MachineCommand mc(
	    FilePath(GetExecPlfDir()).add("od_ProgressViewer").fullPath() );
	mc.addArg( GetPID() );
	//TODO: make this work, it won't work without an actual log file

	strmismine_ = true;
	strm_ = new od_ostream( mc );
	if ( !strm_ || !strm_->isOK() )
	{
	    od_cerr() << name() << ": Cannot open window for output" << od_endl;
	    od_cerr() << "Using std output instead" << od_endl;
	    deleteAndZeroPtr( strm_ );
	    res = 0;
	}
    }

    if ( res != "window" )
    {
	if ( !res.isEmpty() )
	{
	    strmismine_ = true;
	    strm_ = new od_ostream( res );
	}

	if ( !strm_ || !strm_->isOK() )
	{
	    if ( !res.isEmpty() )
	    {
		od_cerr() << name() << ": Cannot open log file" << od_endl;
		od_cerr() << "Using stdoutput instead" << od_endl;
	    }
	    deleteAndZeroPtr( strm_ );
	    strm_ = &od_cout();
	    strmismine_ = false;
	}
    }

    if ( !strm_ || !strm_->isOK() )
	return false;

    if ( comm_ )
	comm_->setStream( *strm_ );

    return true;
}


bool BatchProgram::canStartdoWork() const
{
    if ( !iopar_ )
	return true;

    OS::CommandExecPars jobpars;
    jobpars.usePar( *iopar_ );

    return jobpars.launchtype_ != OS::BatchWait;
}


void BatchProgram::eventLoopStartedCB( CallBacker* )
{
    mDetachCB( timer_->tick, BatchProgram::eventLoopStartedCB );

    od_ostream& logstrm = strm_ ? *strm_ : od_cerr();
    logstrm << "Starting program: " << clparser_->getExecutable();
    if ( clparser_->nrArgs() > 1 )
	logstrm << " '" << name() << "'";
    logstrm << od_newline
	    << "Processing on: " << GetLocalHostName() << od_newline
	    << "Process ID: " << GetPID() << od_endl;
#ifdef __debug__
    if ( clparser_->nrArgs() > 1 )
    {
	logstrm << "Full command: " << clparser_->getExecutable();
	for ( int idx=0; idx<clparser_->nrArgs(); idx++ )
	    logstrm << " " << clparser_->getArg(idx);
	logstrm << od_endl;
    }
#endif

    if ( !isOK() )
    {
	endWorkCB( nullptr );
	return;
    }

    eventLoopStarted.trigger(); // Call to loadModules needs to be external
}


void BatchProgram::modulesLoaded()
{
    //Here we could register any batch program back to od_main
    if ( canStartdoWork() )
    {
	startDoWork.trigger();
	return;
    }

    //Start listening and registers
    const BatchServiceServerMgr& batchmgr = BatchServiceServerMgr::getMgr();
    if ( batchmgr.canReceiveRequests() )
    {
	*strm_ << "\nGoing in waiting mode" << od_endl;
	Threads::Locker lckr( statelock_ );
	status_ = WorkWait;
    }
    else
    {
	*strm_ << "\nStopping an unregistered Batch-Wait application"
	       << od_endl;
	endWorkCB( nullptr );
    }
}


bool BatchProgram::canReceiveRequests() const
{
    return BatchServiceServerMgr::getMgr().canReceiveRequests();
}


void BatchProgram::initWork()
{
    Threads::Locker lckr( statelock_ );
    status_ = WorkStarted;
}


void BatchProgram::startTimer()
{
    mAttachCB( timer_->tick, BatchProgram::workMonitorCB );
    timer_->start( 100, true );
}


void BatchProgram::postWork(bool res)
{
    Threads::Locker lckr( statelock_ );
    status_ = res ? WorkOK : WorkFail;
}


void BatchProgram::workMonitorCB( CallBacker* )
{
    Threads::Locker lckr( statelock_ );
    Status state = status_;
    lckr.unlockNow();
    if ( state == WorkFail || state == WorkOK )
    {
	Threads::Locker trlckr( batchprogthreadlock_,
				Threads::Locker::DontWaitForLock );
	if ( trlckr.isLocked() )
	{
	    doFinalize();
	    trlckr.unlockNow();
	}

	endWorkCB( nullptr );
	return;
    }

    /* Here we can report if alive or paused
    BatchServiceServerMgr& batchmgr = BatchServiceServerMgr::getMgr();
    if ( state == WorkPaused )
    {
	*strm_ << "\nPaused" << od_endl;
    }
    else
    {
	OD::JSON::Object obj;
	obj.set( sKey::Status(), state );
	batchmgr.doSendRequest( sKey::Status(), &obj );
    }
    */

    timer_->start( 5000, true );
}


void BatchProgram::doFinalize()
{
    if ( thread_ )
    {
	thread_->waitForFinish();
	deleteAndZeroPtr( thread_ );
    }
}


void BatchProgram::endWorkCB( CallBacker* cb )
{
    const bool workdoneok = status_ == WorkOK;
    infoMsg( sKeyFinishMsg() );
    if ( comm_ )
    {
	JobCommunic::State s = comm_->state();

	bool isSet =  s == JobCommunic::AllDone
	           || s == JobCommunic::JobError
		   || s == JobCommunic::HostError;

	if ( !isSet )
	    comm_->setState( workdoneok ? JobCommunic::AllDone
					: JobCommunic::HostError );

	const bool ok = comm_->sendState( true );

	if ( ok )	infoMsg( "Successfully wrote final status" );
	else		infoMsg( "Could not write final status" );

	comm_->disConnect();
    }

    killNotify( false );
    endWork.trigger();
    deleteInstance( workdoneok ? 0 : 1 );
}


void BatchProgram::progKilled( CallBacker* )
{
    Threads::Locker lckr( statelock_ );
    status_ = Killed;
    lckr.unlockNow();
    infoMsg( "BatchProgram Killed." );

    if ( comm_ )
    {
	comm_->setState( JobCommunic::Killed );
	comm_->sendState( true );
    }

    killNotify( false );
    killed.trigger(); // deregister. will it work?

#ifdef __debug__
    abort();
#endif
}


void BatchProgram::killNotify( bool yn )
{
    CallBack cb( mCB(this,BatchProgram,progKilled) );

    if ( yn )
	SignalHandling::startNotify( SignalHandling::Kill, cb );
    else
	SignalHandling::stopNotify( SignalHandling::Kill, cb );
}


bool BatchProgram::pauseRequested()
{
    const bool pauserequest = comm_ && comm_->pauseRequested();
    if ( pauserequest )
    {
	Threads::Locker lckr( statelock_ );
	status_ = WorkWait;
	lckr.unlockNow();
	pause.trigger();
    }
    return pauserequest;
}


void BatchProgram::setResumed()
{
    resume.trigger();
}


bool BatchProgram::errorMsg( const uiString& msg, bool cc_stderr )
{
    const bool hasstrm = strm_ && strm_->isOK();
    if ( hasstrm )
	*strm_ << '\n' << ::toString(msg) << '\n' << od_endl;
    if ( cc_stderr || !hasstrm )
	od_cerr() << '\n' << ::toString(msg) << '\n' << od_endl;

    if ( comm_ && comm_->ok() )
	return comm_->sendErrMsg( ::toString(msg) );

    return true;
}


bool BatchProgram::infoMsg( const char* msg, bool cc_stdout )
{
    if ( strm_ && strm_->isOK() )
	*strm_ << '\n' << msg << '\n' << od_endl;
    if ( cc_stdout )
	od_cout() << '\n' << msg << '\n' << od_endl;

    return true;
}
