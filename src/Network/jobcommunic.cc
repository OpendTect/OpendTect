/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : 9-5-2005
 * FUNCTION : Multi-machine batch communicator.
-*/


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

#include "hiddenparam.h"

class sendData
{
public:
	sendData(char tag,int stat,const char* msg)
	    : tag_(tag) , status_(stat), msg_(msg)	{}

    char		tag_;
    int			status_;
    BufferString	msg_;
};

static	HiddenParam<JobCommunic,int> jobcommsendretmgr_( 0 );
static HiddenParam<JobCommunic,Threads::Lock*> jobcommlockmgr_( nullptr );
static HiddenParam<JobCommunic,Threads::Lock*> jobcommsendmsglockmgr_( nullptr);


JobCommunic::JobCommunic( const char* host, PortNr_Type port, int jid )
    : masterauth_(System::hostAddress(host),port)
    , timestamp_(Time::getMilliSeconds())
    , stillok_(true)
    , nrattempts_(0)
    , maxtries_ (GetEnvVarIVal("DTECT_MM_MSTR_RETRY",10))
    , socktimeout_(GetEnvVarIVal("DTECT_MM_CL_SOCK_TO",2000))
    , min_time_between_update_(1000 * GetEnvVarIVal("DTECT_MM_INTRVAL",10))
    , failtimeout_(1000 * GetEnvVarIVal("DTECT_MM_CL_FAIL_TO",30))
    , pausereq_(false)
    , jobid_(jid)
    , lastsucces_(Time::getMilliSeconds())
    , logstream_(createLogStream())
{
    jobcommsendretmgr_.setParam( this, 0 );
    jobcommlockmgr_.setParam( this, new Threads::Lock(true) );
    jobcommsendmsglockmgr_.setParam( this, new Threads::Lock(true) );
    min_time_between_msgupdates_ =
				1000 * GetEnvVarIVal( "DTECT_MM_INTRVAL", 1 );
    lastupdate_ = timestamp_;
    dumpSystemInfo();
    socket_ = new Network::Socket( false, false );
    socket_->setTimeout( socktimeout_ );

    const bool ret = socket_->connectToHost( primaryAuthority() );
    BufferString logmsg( "Connection to", primaryAuthority().getHost(),
			 " port " );
    logmsg.add( primaryAuthority().getPort() ).add( " : " );
    logMsg( ret, logmsg, !ret ? "" :socket_->errMsg().getFullString() );
}


JobCommunic::JobCommunic( const char* host, PortNr_Type port, int jid,
			  StreamData& )
    : masterauth_(System::hostAddress(host),port)
    , timestamp_(Time::getMilliSeconds())
    , stillok_(true)
    , nrattempts_(0)
    , maxtries_ (GetEnvVarIVal("DTECT_MM_MSTR_RETRY",10))
    , socktimeout_(GetEnvVarIVal("DTECT_MM_CL_SOCK_TO",2000))
    , min_time_between_update_(1000 * GetEnvVarIVal("DTECT_MM_INTRVAL",10))
    , failtimeout_(1000 * GetEnvVarIVal("DTECT_MM_CL_FAIL_TO",30))
    , pausereq_(false)
    , jobid_(jid)
    , lastsucces_(Time::getMilliSeconds())
    , logstream_(createLogStream())
{
    jobcommsendretmgr_.setParam( this, 0 );
    jobcommlockmgr_.setParam( this, new Threads::Lock(true) );
    jobcommsendmsglockmgr_.setParam( this, new Threads::Lock(true) );
    min_time_between_msgupdates_ =
				1000 * GetEnvVarIVal( "DTECT_MM_INTRVAL", 1 );
    lastupdate_ = timestamp_;
    dumpSystemInfo();
    socket_ = new Network::Socket( false, false );
    socket_->setTimeout( socktimeout_ );

    const bool ret = socket_->connectToHost( primaryAuthority() );
    BufferString logmsg( "Connection to", primaryAuthority().getHost(),
			 " port " );
    logmsg.add( primaryAuthority().getPort() ).add( " : " );
    logMsg( ret, logmsg, !ret ? "" :socket_->errMsg().getFullString() );
}


JobCommunic::~JobCommunic()
{
    detachAllNotifiers();
    clearHiddenParams();
    delete socket_;
    delete logstream_;
}

void JobCommunic::clearHiddenParams()
{
    jobcommsendretmgr_.removeParam( this );
    jobcommlockmgr_.removeAndDeleteParam( this );
    jobcommsendmsglockmgr_.removeAndDeleteParam( this );
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


void JobCommunic::setTimeBetweenMsgUpdates( int ms )
{
    min_time_between_msgupdates_ = ms;
}


bool JobCommunic::updateMsg( char tag , int status, const char* msg )
{
    const int elapsed_succ = Time::passedSince( lastsucces_ );
    const int elapsed_atmpt = Time::passedSince( timestamp_ );

    if ( elapsed_succ < 0 || elapsed_atmpt < 0 )
        UsrMsg( "System clock skew detected (Ignored)." );
    else if ( elapsed_succ < min_time_between_update_ || elapsed_atmpt < 500 )
    {
        checkPrimaryHostTimeout();
        return true;
    }

    if ( Time::passedSince(lastupdate_) < min_time_between_msgupdates_ )
	return true;

    lastupdate_ = Time::getMilliSeconds();
    return sendMsg( tag , status, msg );
}

BufferString JobCommunic::buildString( char tag , int status, const char* msg )
{
    FileMultiString statstr;
    statstr += jobid_;
    statstr += status;
    statstr += BufferString( System::localFullHostName() );
    statstr += GetPID();

    if ( msg && *msg )
	statstr += msg;

    char tagstr[3];
    tagstr[0] = tag;
    tagstr[1] = statstr.sepChar();
    tagstr[2] = '\0';
    BufferString buf( tagstr );
    buf += statstr;
    return buf;
}


bool JobCommunic::sendMsg( char tag , int status, const char* msg )
{
    Threads::Lock& lock_ = *jobcommlockmgr_.getParam( this );
    Threads::Locker lckr( lock_ );
    CBCapsule<sendData> caps( sendData(tag,status,msg), this );
    sendMsgCB( &caps );

    const bool sendret_ = jobcommsendretmgr_.getParam( this ) == 1;
    return sendret_;
}


void JobCommunic::sendMsgCB( CallBacker* cber )
{
    Threads::Lock& sendmsglock_ = *jobcommsendmsglockmgr_.getParam( this );
    Threads::Locker lckr( sendmsglock_ );
    mDynamicCastGet( CBCapsule<sendData>*, caps, cber )
    mEnsureExecutedInMainThreadWithCapsule( JobCommunic::sendMsgCB, caps );
    mCBCapsuleUnpack( sendData, sdata, caps );

    BufferString& msg = sdata.msg_;
    BufferString buf = buildString( sdata.tag_, sdata.status_, msg );
    if ( DBG::isOn(DBG_MM) )
    {
	BufferString dbmsg( "JobCommunic::sendMsg -- sending : " );
	dbmsg += buf;
	DBG::message( dbmsg );
    }
    bool writestat = socket_->write( buf );
    BufferString logmsg( "Writing to socket ", buf );
    logMsg( writestat, logmsg,
	    !writestat ? socket_->errMsg().getFullString().str() : "" );

    char primaryhostinfo;
    BufferString inp;
    const bool readstat = socket_->read( inp );
    logMsg( readstat, "Reading from socket", inp );

    primaryhostinfo = inp[0];
    bool ret = !inp.isEmpty();
    bool appexit = false;
    if ( !ret )
    {
	BufferString emsg( "Error reading from Primary Host: ",
			    socket_->errMsg().getFullString() );
	setErrMsg( emsg );
    }
    else if ( primaryhostinfo == mRSP_WORK )
	pausereq_ = false;

    else if ( primaryhostinfo == mRSP_PAUSE )
	pausereq_ = true;
    else if ( primaryhostinfo == mRSP_STOP )
    {
	buf = buildString( sdata.tag_, mSTAT_KILLED, msg );
	writestat = socket_->write( buf );
	logmsg = "Writing to socket "; logmsg += buf;
	logMsg( writestat, logmsg,
	    !writestat ? socket_->errMsg().getFullString().str() : "" );
	directMsg( "Exiting on request of Primary Host." );
	clearHiddenParams();
	ApplicationData::exit( -1 );
	appexit = true;
    }
    else
    {
	BufferString emsg( "Primary Host sent an unkown response code: '" );
	emsg.add( primaryhostinfo ).add( "', " )
	    .add( socket_->errMsg().getFullString() );
	setErrMsg( emsg );
	ret = false;
    }

    if ( !appexit )
	jobcommsendretmgr_.setParam( this, ret ? 1 : 0 );
}


void JobCommunic::checkMasterTimeout()
{
    checkPrimaryHostTimeout();
}


Network::Authority& JobCommunic::primaryAuthority()
{
    return masterauth_;
}


void JobCommunic::checkPrimaryHostTimeout()
{
    const int elapsed = Time::passedSince( lastsucces_ );

    if ( elapsed>0 && elapsed>failtimeout_ )
    {
	BufferString msg( "Time-out contacting Primary Host. Last contact " );
	msg.add( elapsed/1000 ).add( " sec ago. Exiting." );
	directMsg( msg );
	ApplicationData::exit( -1 );
    }
}


void JobCommunic::directMsg( const char* msg )
{
    od_ostream& strm = strm_ && strm_->isOK() ? *strm_ : od_cout();
    strm << msg << od_endl;
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

    BufferString fnm( "od_mmproc_", primaryAuthority().getHost(), "_" );
    fnm.add( jobid_ ).replace( '.', '_' );
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

    *logstream_ << "----------------------------------------------";
    *logstream_ << "\nLocal Host Name    : " << System::localHostName();
    *logstream_ << "\nLocal Host Address : " << System::localAddress();
    *logstream_ << "\nServer Address     : " << primaryAuthority().getHost();
    *logstream_ << "\nServer Port        : " << primaryAuthority().getPort();
    *logstream_ << "\n-----------------------------------------------"
		<< od_endl;
}
