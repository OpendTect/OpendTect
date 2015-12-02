/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : 9-5-2005
 * FUNCTION : Multi-machine batch communicator.
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "jobcommunic.h"

#include "debug.h"
#include "envvars.h"
#include "hostdata.h"
#include "oddirs.h"
#include "separstr.h"
#include "strmdata.h"
#include "systeminfo.h"
#include "netsocket.h"
#include "timefun.h"
#include "msgh.h"

#include <iostream>
#include "mmcommunicdefs.h"

JobCommunic::JobCommunic( const char* host, int port, int jid,
			  StreamData& sout )
    : masterhost_(System::hostAddress(host))
    , masterport_(port)
    , timestamp_(Time::getMilliSeconds())
    , stillok_(true)
    , nrattempts_(0)
    , maxtries_ (GetEnvVarIVal("DTECT_MM_MSTR_RETRY",10))
    , socktimeout_(GetEnvVarIVal("DTECT_MM_CL_SOCK_TO",2000))
    , min_time_between_update_(1000 * GetEnvVarIVal("DTECT_MM_INTRVAL",10))
    , failtimeout_(1000 * GetEnvVarIVal("DTECT_MM_CL_FAIL_TO",30))
    , pausereq_(false)
    , jobid_(jid)
    , sdout_(sout)
    , lastsucces_(Time::getMilliSeconds())
    , logstream_(createLogStream())
{
    dumpSystemInfo();
    socket_ = new Network::Socket( false );
    socket_->setTimeout( socktimeout_ );
    
    const bool ret = socket_->connectToHost( masterhost_, masterport_ );
    BufferString logmsg( "Connection to", masterhost_, " port " );
    logmsg.add( masterport_ ).add( " : " );
    logMsg( ret, logmsg, !ret ? "" :socket_->errMsg().getFullString() );
}


JobCommunic::~JobCommunic()
{
    delete socket_;
    delete logstream_;
}


bool JobCommunic::sendErrMsg_( const char* msg )
{
    const bool ret = sendMsg( mERROR_MSG, -1, msg );
    logMsg( ret, "Send error message", msg );
    return ret;
}


bool JobCommunic::sendPID_( int pid )
{
    const bool ret = sendMsg( mPID_TAG, pid );
    logMsg( ret, "Send PID", BufferString( "", pid ) );
    return ret;
}


bool JobCommunic::sendProgress_( int progress, bool immediate )
{
    if ( immediate )
	return sendMsg( mPROC_STATUS, progress );

    return updateMsg( mPROC_STATUS, progress );
}


bool JobCommunic::sendState_( State st, bool isexit, bool immediate )
{
    int _stat = mSTAT_UNDEF;
    switch( st )
    {
	case Working	: _stat = mSTAT_WORKING; break;
	case WrapUp	: _stat = mSTAT_WRAPUP; break;
	case Finished	: _stat = mSTAT_FINISHED; break;
	case AllDone	: _stat = mSTAT_ALLDONE; break;
	case Paused	: _stat = mSTAT_PAUSED; break;
	case JobError	: _stat = mSTAT_JOBERROR; break;
	case HostError	: _stat = mSTAT_HSTERROR; break;
	case Killed	: _stat = mSTAT_KILLED; break;
	case Timeout	: _stat = mSTAT_TIMEOUT; break;
	default		: _stat = mSTAT_UNDEF; break;
    }


    if ( immediate )
	return sendMsg( isexit ? mEXIT_STATUS : mCTRL_STATUS, _stat );

    return updateMsg( isexit ? mEXIT_STATUS : mCTRL_STATUS, _stat );
}


bool JobCommunic::updateMsg( char tag , int status, const char* msg )
{
    const int elapsed_succ = Time::passedSince( lastsucces_ );
    const int elapsed_atmpt = Time::passedSince( timestamp_ );

    if ( elapsed_succ < 0 || elapsed_atmpt < 0 )
        UsrMsg( "System clock skew detected (Ignored)." );
    else if ( elapsed_succ < min_time_between_update_ || elapsed_atmpt < 500 )
    {
        checkMasterTimeout();
        return true;
    }

    return sendMsg( tag , status, msg );
}


bool JobCommunic::sendMsg( char tag , int status, const char* msg )
{
    FileMultiString statstr;

    statstr += jobid_;
    statstr += status;
    statstr += HostData::localHostName();
    statstr += GetPID();

    if ( msg && *msg )
	statstr += msg;

    if ( DBG::isOn(DBG_MM) )
    {
	BufferString dbmsg( "JobCommunic::sendMsg -- sending : " );
	dbmsg += statstr;
	DBG::message( dbmsg );
    }

    char tagstr[3];
    tagstr[0] = tag;
    tagstr[1] = statstr.sepChar();
    tagstr[2] = '\0';
    BufferString buf( tagstr );
    buf += statstr;
    const bool writestat = socket_->write( buf );
    const BufferString logmsg( "Writing to socket ", buf );
    logMsg( writestat, logmsg,
	    !writestat ? socket_->errMsg().getFullString().str() : "" );

    char masterinfo;
    BufferString inp;
    const bool readstat = socket_->read( inp );
    logMsg( readstat, "Reading from socket", inp );

    masterinfo = inp[0];
    bool ret = !inp.isEmpty();
    if ( !ret )
    {
	BufferString emsg( "Error reading from Master: ",
			    socket_->errMsg().getFullString() );
	setErrMsg( emsg );
    }

    else if ( masterinfo == mRSP_WORK )
	pausereq_ = false;

    else if ( masterinfo == mRSP_PAUSE )
	pausereq_ = true;

    else if ( masterinfo == mRSP_STOP )
    {
	directMsg( "Exiting on request of Master." );
	ExitProgram( -1 );
    }
    else
    {
	BufferString emsg( "Master sent an unkown response code: ",
			    socket_->errMsg().getFullString() );
	setErrMsg( emsg );
	ret = false;
    }

    return ret;
}


void JobCommunic::checkMasterTimeout()
{
    const int elapsed = Time::passedSince( lastsucces_ );

    if ( elapsed>0 && elapsed>failtimeout_ )
    {
	BufferString msg( "Time-out contacting master. Last contact " );
	msg.add( elapsed/1000 ).add( " sec ago. Exiting." );
	directMsg( msg );
	ExitProgram( -1 );
    }
}


void JobCommunic::directMsg( const char* msg )
{
    (sdout_.ostrm ? *sdout_.ostrm : std::cerr) << msg << std::endl;
    logMsg( true, msg, "" );
}


void JobCommunic::alarmHndl(CallBacker*)
{
    // no need to do anything - see comments at JobIOHandler::alarmHndl
    UsrMsg( "MM Socket Communication: time-out." );
    logMsg( false, "MM Socket Communication: time-out.", "" );
}


void JobCommunic::disConnect()
{
    socket_->disconnectFromHost();
}


void JobCommunic::setErrMsg( const char* m )
{
    errmsg_ = tr("[%1]: %2").arg(GetPID()).arg(m);
    logMsg( false, errmsg_.getFullString(), "" );
}


od_ostream* JobCommunic::createLogStream()
{
    if ( !DBG::isOn() )
	return 0;

    BufferString fnm( "od_mmproc_", masterhost_, "_" ); fnm.add( jobid_ );
    fnm.replace( '.', '_' );
    FilePath logfp( FilePath::getTempDir(), fnm );
    logfp.setExtension( ".log" );
    return new od_ostream( logfp.fullPath() );
}


void JobCommunic::logMsg( bool stat, const char* msg, const char* details )
{
    if ( !logstream_ )
	return;

    BufferString finalmsg = stat ? "Success: " : "Failure: ";
    finalmsg.add( msg ).add( " : " ).add( details );
    *logstream_ << finalmsg << od_endl;
}


void JobCommunic::dumpSystemInfo()
{
    if ( !logstream_ )
	return;

    *logstream_ << "----------------------------------------------"  <<od_endl;
    *logstream_ << "Local Host Name    : " << System::localHostName()<<od_endl;
    *logstream_ << "Local Host Address : " << System::localAddress() <<od_endl;
    *logstream_ << "Server Address     : " << masterhost_	     <<od_endl;
    *logstream_ << "-----------------------------------------------" <<od_endl;
}
