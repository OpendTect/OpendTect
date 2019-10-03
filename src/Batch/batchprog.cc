/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "envvars.h"
#include "commandlineparser.h"
#include "file.h"
#include "genc.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "strmprov.h"
#include "filepath.h"
#include "sighndl.h"
#include "threadwork.h"
#include "hostdata.h"
#include "plugins.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "ascstream.h"
#include "debug.h"
#include "oscommand.h"
#include "winutils.h"
#include <iostream>

#ifndef __win__
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
    ApplicationData::exit( retcode );
}


BatchProgram::BatchProgram()
    : NamedObject("")
    , stillok_(false)
    , inbg_(false)
    , sdout_(*new StreamData)
    , iopar_(new IOPar)
    , comm_(0)
    , clparser_(0)
{
#ifdef __win__
    disableAutoSleep();
#endif
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

    inbg_ = clparser_->hasKey( sKeyBG() );

    BufferString masterhost;
    clparser_->getVal( sKeyMasterHost(), masterhost );

    int masterport = -1;
    clparser_->getVal( sKeyMasterPort(), masterport );

    clparser_->getVal( sKeyJobID(), jobid_ );

    if ( masterhost.size() && masterport > 0 )  // both must be set.
    {
	comm_ = new JobCommunic( masterhost, masterport, jobid_, sdout_ );
	Threads::WorkManager::twm().setQuickStop( true );
    }

    BufferStringSet normalargs;
    clparser_->getNormalArguments( normalargs );

    BufferString parfilnm;
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
	iopar_->set( sKey::LogFile(), StreamProvider::sStdErr() );

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

    sdout_.close();
    delete &sdout_;
    delete clparser_;

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
    if ( sdout_.ostrm )
	*sdout_.ostrm << '\n' << msg.getFullString() << '\n' << std::endl;

    if ( !sdout_.ostrm || cc_stderr )
	std::cerr << '\n' << msg.getFullString() << '\n' << std::endl;

    if ( comm_ && comm_->ok() ) return comm_->sendErrMsg(msg.getFullString());

    return true;
}


bool BatchProgram::infoMsg( const char* msg, bool cc_stdout )
{
    if ( sdout_.ostrm )
	*sdout_.ostrm << '\n' << msg << '\n' << std::endl;

    if ( !sdout_.ostrm || cc_stdout )
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

    bool hasviewprogress = true;
#ifdef __cygwin__
    hasviewprogress = false;
#endif

    if ( hasviewprogress && res && res=="window" )
    {
	BufferString cmd = "@";
	cmd += "\"";
	cmd += FilePath(GetExecPlfDir()).add("od_ProgressViewer").fullPath();
	cmd += "\" ";

	cmd += GetPID();
	StreamProvider sp( cmd );
	sdout_ = sp.makeOStream();
	if ( !sdout_.usable() )
	{
	    std::cerr << name() << ": Cannot open window for output"<<std::endl;
	    std::cerr << "Using std output instead" << std::endl;
	    res = 0;
	}
    }

    if ( res != "window" )
    {
	if ( res.isEmpty() )
	    res = StreamProvider::sStdErr();
	sdout_ = StreamProvider(res).makeOStream();
	if ( !sdout_.ostrm )
	{
	    std::cerr << name() << ": Cannot open log file" << std::endl;
	    std::cerr << "Using stderror instead" << std::endl;
	    sdout_.ostrm = &std::cerr;
	}
    }

    stillok_ = sdout_.usable();
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
	*sdout_.ostrm << errmsg.buf() << std::endl;

    return ioobj;
}
