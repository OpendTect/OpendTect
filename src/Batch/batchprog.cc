/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "ascstream.h"
#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "genc.h"
#include "hostdata.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "oscommand.h"
#include "plugins.h"
#include "sighndl.h"
#include "strmprov.h"
#include "threadwork.h"
#include "odservicebase.h"
#include "timer.h"
#include "odbatchservice.h"


#include "netserver.h"

#ifdef __win__
# include "winutils.h"
#else
# include "_execbatch.h"
#endif

#include "oddirs.h"

const char* BatchProgram::sKeyMasterHost()
{ return OS::MachineCommand::sKeyMasterHost(); }
const char* BatchProgram::sKeyMasterPort()
{ return OS::MachineCommand::sKeyMasterPort(); }
const char* BatchProgram::sKeyBG()
{ return OS::MachineCommand::sKeyBG(); }
const char* BatchProgram::sKeyJobID()
{ return OS::MachineCommand::sKeyJobID(); }

BatchProgram* BatchProgram::inst_ = 0;

BatchProgram& BP()
{
    if ( !BatchProgram::inst_ )
	BatchProgram::inst_ = new BatchProgram;
    return *BatchProgram::inst_;
}


void BatchProgram::deleteInstance( int retcode )
{
    delete BatchProgram::inst_;
    IOM().applClosing();
    ApplicationData::exit( retcode );
}


BatchProgram::BatchProgram()
    : NamedObject("")
    , iopar_(new IOPar)
    , programStarted(this)
    , startDoWork(this)
{
    timer_ = new Timer( "Updating" );
    mAttachCB( timer_->tick, BatchProgram::eventLoopStarted );
    ODBatchService& service = ODBatchService::getMgr();

    mAttachCB( service.externalAction, BatchProgram::doWorkCB );

    if ( !timer_->isActive() )
	timer_->start(0, true);
#ifdef __win__
    disableAutoSleep();
#endif
}


void BatchProgram::eventLoopStarted( CallBacker* cb )
{
    if ( isStartDoWork() )
	timer_->stop();

    const BufferString* flnm = new BufferString( strm_->fileName() );
    programStarted.trigger( flnm );

    if ( isStartDoWork() )
    {
	const int ret = BP().isStillOK() ? 0 : 1;
	BP().deleteInstance( ret );
    }
}


void BatchProgram::doWorkCB( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,actstr,cb);

    if ( FixedString(actstr) != ODServiceBase::sKeyTransferCmplt() )
	return;
    const BufferString* logflnm = new BufferString( fp_.fullPath(),
								"_log.txt" );
    startDoWork.trigger( logflnm );
    const int ret = stillok_ ? 0 : 1;
    deleteInstance( ret );
}


void BatchProgram::init()
{
    delete clparser_;

    OD::ModDeps().ensureLoaded( "Batch" );

    clparser_ = new CommandLineParser;
    clparser_->setKeyHasValue( sKeyMasterHost() );
    clparser_->setKeyHasValue( sKeyMasterPort() );
    clparser_->setKeyHasValue( sKeyJobID() );
    clparser_->setKeyHasValue( sKeyDataDir() );
    clparser_->setKeyHasValue( OS::CommandExecPars::sKeyPriority() );
    clparser_->setKeyHasValue(	"odserver" );
    clparser_->setKeyHasValue( Network::Server::sKeyPort() );

    inbg_ = clparser_->hasKey( sKeyBG() );

    BufferString masterhost;
    clparser_->getVal( sKeyMasterHost(), masterhost );

    int masterport = -1;
    clparser_->getVal( sKeyMasterPort(), masterport );

    clparser_->getVal( sKeyJobID(), jobid_ );

    if ( masterhost.size() && masterport > 0 )  // both must be set.
    {
	comm_ = new JobCommunic( masterhost, masterport, jobid_ );
	Threads::WorkManager::twm().setQuickStop( true );
    }

    BufferStringSet normalargs;
    clparser_->getNormalArguments( normalargs );
    BufferString launchtype;
    clparser_->getVal( sKey::LaunchType(), launchtype );

    startdoworknow_ = launchtype.isEqual( sKey::Batch() );

    BufferString parfilnm;
    for ( int idx=normalargs.size()-1; idx>=0; idx-- )
    {
	const FilePath parfp( normalargs.get(idx) );

	parfilnm = parfp.fullPath();
	parfilnm.replace( '%', ' ' );
	if ( File::exists(parfilnm) )
	{
	    fp_ = parfilnm;
	    fp_.setExtension( 0 );
	    break;
	}

	parfilnm.setEmpty();
    }

    const bool simplebatch = clparser_->hasKey( sKeySimpleBatch() );
    if ( parfilnm.isEmpty() && !simplebatch )
    {
	errorMsg( tr("%1: No existing parameter file name specified")
		 .arg(clparser_->getExecutableName()) );
	return;
    }

    if ( !simplebatch )
    {
	setName( parfilnm );
	od_istream odstrm(parfilnm);
	if ( !odstrm.isOK() )
	{
	    errorMsg( tr("%1: Cannot open parameter file: %2" )
		.arg(clparser_->getExecutableName() )
		.arg(parfilnm) );
	    return;
	}

	ascistream aistrm( odstrm, true );
	if ( sKey::Pars() != aistrm.fileType() )
	{
	    errorMsg( tr("%1: Input file %2 is not a parameter file")
		.arg(clparser_->getExecutableName())
		.arg(parfilnm) );

	    od_cerr() << aistrm.fileType() << od_endl;
	    return;
	}

	iopar_->getFrom( aistrm );
	odstrm.close();
    }

    if ( iopar_->size() == 0 && !simplebatch )
    {
	errorMsg( tr( "%1: Invalid input file %2")
		    .arg( clparser_->getExecutableName() )
		    .arg( parfilnm) );
        return;
    }

    BufferString res = iopar_->find( sKey::LogFile() );
    if ( !res )
	iopar_->set( sKey::LogFile(), od_stream::sStdIO() );

    res = iopar_->find( sKey::DataRoot() );
    if ( !res.isEmpty() && File::exists(res) )
	SetEnvVar( __iswin__ ? "DTECT_WINDATA" : "DTECT_DATA", res );

    if ( clparser_->getVal(sKeyDataDir(),res) && !res.isEmpty() &&
	 File::exists(res) )
	SetEnvVar( "DTECT_DATA", res );

    res = iopar_->find( sKey::Survey() );

    if ( simplebatch && clparser_->getVal(sKeySurveyDir(), res) )
	iopar_->set( sKey::Survey(), res );
    else if ( res.isEmpty() )
	IOMan::newSurvey();
    else
    {
	if ( DBG::isOn(DBG_PROGSTART) )
	{
	    const char* oldsnm = IOM().surveyName();
	    if ( !oldsnm ) oldsnm = "<empty>";
	    if ( res!=oldsnm )
	    {
		BufferString msg( "Using survey from par file: ", res,
				  ". was: " ); msg += oldsnm;
		infoMsg( msg );
	    }
	}
	IOMan::setSurvey( res );
    }

    killNotify( true );

    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
#ifdef __win__
    enableAutoSleep();
#endif
    infoMsg( sKeyFinishMsg() );
    IOM().applClosing();

    if ( comm_ )
    {

	JobCommunic::State s = comm_->state();

	bool isSet =  s == JobCommunic::AllDone
	           || s == JobCommunic::JobError
		   || s == JobCommunic::HostError;

	if ( !isSet )
	    comm_->setState( stillok_ ? JobCommunic::AllDone
				    : JobCommunic::HostError );

	bool ok = comm_->sendState( true );

	if ( ok )	infoMsg( "Successfully wrote final status" );
	else		infoMsg( "Could not write final status" );

	comm_->disConnect();
    }

    killNotify( false );

    strm_->close();
    if ( strmismine_ )
	delete strm_;
    strm_ = nullptr;
    deleteAndZeroPtr( clparser_ );

    // Do an explicit exitProgram() here, so we are sure the program
    // is indeed ended and we won't get stuck while cleaning up things
    // that we don't care about.
    ExitProgram( stillok_ ? 0 : 1 );

    // These will never be reached...
    delete iopar_;
    delete comm_;
}


void BatchProgram::progKilled( CallBacker* )
{
    infoMsg( "BatchProgram Killed." );

    if ( comm_ )
    {
	comm_->setState( JobCommunic::Killed );
	comm_->sendState( true );
    }

    killNotify( false );

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


bool BatchProgram::pauseRequested() const
    { return comm_ ? comm_->pauseRequested() : false; }


bool BatchProgram::errorMsg( const uiString& msg, bool cc_stderr )
{
    if ( strm_ && strm_->isOK() )
	*strm_ << '\n' << msg.getFullString() << '\n' << od_endl;
    if ( cc_stderr )
	od_cerr() << '\n' << msg.getFullString() << '\n' << od_endl;

    if ( comm_ && comm_->ok() )
	return comm_->sendErrMsg(msg.getFullString());

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



bool BatchProgram::initOutput()
{
    stillok_ = false;
    if ( comm_ && !comm_->sendPID(GetPID()) )
    {
	errorMsg( tr("Could not contact master. Exiting."), true );
	exit( 0 );
    }

    BufferString res = pars().find( sKey::LogFile() );
    if ( res == "stdout" ) res.setEmpty();

    if ( strmismine_ )
	delete strm_;

    bool hasviewprogress = true;
#ifdef __cygwin__
    hasviewprogress = false;
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

    stillok_ = strm_ && strm_->isOK();
    if ( comm_ && strm_ )
	comm_->setStream( *strm_ );
    if ( stillok_ )
	PIM().loadAuto( true );
    return stillok_;
}


IOObj* BatchProgram::getIOObjFromPars(	const char* bsky, bool mknew,
					const IOObjContext& ctxt,
					bool msgiffail ) const
{
    BufferString errmsg;
    IOObj* ioobj = IOM().getFromPar( pars(), bsky, ctxt, mknew, errmsg );
    if ( !ioobj && msgiffail && !errmsg.isEmpty() )
    {
	if ( errmsg.isEmpty() )
	    errmsg.set( "Error getting info from database" );
	if ( strm_ )
	    *strm_ << errmsg << od_endl;
    }

    return ioobj;
}
