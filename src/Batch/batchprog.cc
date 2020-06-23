/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/


#include "batchprog.h"

#include "ascstream.h"
#include "commandlineparser.h"
#include "dbman.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "ioobjctxt.h"
#include "iopar.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "plugins.h"
#include "sighndl.h"
#include "od_ostream.h"

#ifndef __win__
# include "_execbatch.h"
#endif

#include "oddirs.h"


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
    DBM().applClosing();
    ApplicationData::exit( retcode );
}


BatchProgram::BatchProgram()
    : NamedCallBacker("Batch Program")
    , iopar_(new IOPar)
{
    DisableAutoSleep();
}

void BatchProgram::init()
{
    delete clparser_;

    OD::ModDeps().ensureLoaded( "Batch" );

    clparser_ = new CommandLineParser;
    clparser_->setKeyHasValue( OS::MachineCommand::sKeyMasterHost() );
    clparser_->setKeyHasValue( OS::MachineCommand::sKeyMasterPort() );
    clparser_->setKeyHasValue( OS::MachineCommand::sKeyJobID() );
    clparser_->setKeyHasValue( sKeyDataDir() );
    clparser_->setKeyHasValue( OS::CommandExecPars::sKeyPriority() );

    inbg_ = clparser_->hasKey( OS::MachineCommand::sKeyBG() );

    BufferStringSet normalargs;
    clparser_->getNormalArguments( normalargs );

#   define mGetKeyedVal(ky,val) \
    clparser_->setKeyHasValue( OS::MachineCommand::ky() ); \
    clparser_->getKeyedInfo( OS::MachineCommand::ky(), val )

    BufferString masterhost;
    int masterport = -1;
    mGetKeyedVal( sKeyMasterHost, masterhost );
    mGetKeyedVal( sKeyMasterPort, masterport );
    mGetKeyedVal( sKeyJobID, jobid_ );

    if ( masterhost.size() && masterport > 0 )  // both must be set.
	comm_ = new JobCommunic( masterhost, masterport, jobid_ );

    BufferString parfilnm;
    for ( int idx=normalargs.size()-1; idx>=0; idx-- )
    {
	const File::Path parfp( normalargs.get(idx) );

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

    setName( parfilnm );
    if ( !parfilnm.isEmpty() )
    {
	od_istream odstrm( parfilnm );
	if ( !odstrm.isOK() )
	{
	    errorMsg( tr("%1: Cannot open parameter file: %2")
			.arg( clparser_->getExecutableName() )
			.arg( parfilnm ));
	    return;
	}

	ascistream aistrm( odstrm, true );
	if ( sKey::Pars() != aistrm.fileType() )
	{
	    errorMsg( tr("%1: Input file %2 is not a parameter file")
			.arg( clparser_->getExecutableName() )
			.arg( parfilnm ));
	    od_cerr() << aistrm.fileType() << od_endl;
	    return;
	}

	iopar_->getFrom( aistrm );
	odstrm.close();
    }

    if ( iopar_->size() == 0 && !simplebatch )
    {
	errorMsg( tr("%1: Invalid input file %2")
		    .arg( clparser_->getExecutableName() )
		    .arg( parfilnm) );
        return;
    }

    BufferString res = iopar_->find( sKey::LogFile() );
    if ( !res )
	iopar_->set( sKey::LogFile(), od_stream::sStdIO() );

#define mSetDataRootVar(str) \
	SetEnvVar( __iswin__ ? "DTECT_WINDATA" : "DTECT_DATA", str );

    clparser_->setKeyHasValue( sKeyDataDir() );
    clparser_->setKeyHasValue( sKeySurveyDir() );
    if ( clparser_->getKeyedInfo(sKeyDataDir(),res) && File::isDirectory(res) )
    {
	mSetDataRootVar( res );
	iopar_->set( sKey::DataRoot(), res );
    }

    if ( simplebatch && clparser_->getKeyedInfo(sKeySurveyDir(),res) )
	iopar_->set( sKey::Survey(), res );
    else if ( !iopar_->get(sKey::Survey(),res) )
    {
	errorMsg( tr("Invalid parameter file %1\nSurvey key is missing.")
			.arg( parfilnm ) );
	return;
    }

    uiRetVal uirv = DBM().setDataSource( *iopar_ );
    if ( !uirv.isOK() )
	{ errorMsg( uirv ); return; }

    killNotify( true );
    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    EnableAutoSleep();

    infoMsg( sKeyFinishMsg() );
    DBM().applClosing();

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
	*strm_ << '\n' << toString(msg) << '\n' << od_endl;
    if ( cc_stderr )
	od_cerr() << '\n' << toString(msg) << '\n' << od_endl;

    if ( comm_ && comm_->ok() )
	return comm_->sendErrMsg( toString(msg) );

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

    if ( res && res=="window" )
    {
	OS::MachineCommand mc(
	    File::Path(GetExecPlfDir()).add("od_ProgressViewer").fullPath() );
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
    uiString errmsg;
    IOObj* ioobj = DBM().getFromPar( pars(), bsky, ctxt, mknew, errmsg );
    if ( !ioobj && msgiffail )
    {
	if ( errmsg.isEmpty() )
	    errmsg = tr("Error getting info from database");
	if ( strm_ )
	    *strm_ << toString(errmsg) << od_endl;
    }

    return ioobj;
}
