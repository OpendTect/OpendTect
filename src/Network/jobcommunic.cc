/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : 9-5-2005
 * FUNCTION : Multi-machine batch communicator.
-*/
 
static const char* rcsID = "$Id: jobcommunic.cc,v 1.5 2011/07/04 04:37:27 cvsranojay Exp $";

#include "jobcommunic.h"

#include "debugmasks.h"
#include "envvars.h"
#include "errh.h"
#include "hostdata.h"
#include "mmdefs.h"
#include "oddirs.h"
#include "separstr.h"
#include "strmdata.h"
#include "systeminfo.h"
#include "tcpsocket.h"
#include "timefun.h"

#include <iostream>


JobCommunic::JobCommunic( const char* host, int port, int jid,
			  StreamData& sout )
    : masterhost_(System::hostAddress(host))
    , masterport_( port )
    , timestamp_( Time::getMilliSeconds() )
    , stillok_( true )
    , nrattempts_( 0 )
    , maxtries_ ( GetEnvVarIVal("DTECT_MM_MSTR_RETRY",10) )
    , socktimeout_ ( GetEnvVarIVal("DTECT_MM_CL_SOCK_TO",20) )
    , min_time_between_update_(1000 * GetEnvVarIVal("DTECT_MM_INTRVAL",10) )
    , failtimeout_ ( 1000 * GetEnvVarIVal("DTECT_MM_CL_FAIL_TO",300) )
    , pausereq_ ( false )
    , jobid_( jid )
    , sdout_( sout )
    , lastsucces_( Time::getMilliSeconds() )
{
    socket_ = new TcpSocket(); 
    socket_->connectToHost( masterhost_, masterport_ );
    socket_->waitForConnected( -1 );
}


bool JobCommunic::sendErrMsg_( const char* msg )
{
    return sendMsg( mERROR_MSG, -1, msg ); 
}


bool JobCommunic::sendPID_( int pid )
{ 
    return sendMsg( mPID_TAG, pid );
}


bool JobCommunic::sendProgress_( int progress, bool immediate )
{ 
    if ( immediate ) return sendMsg( mPROC_STATUS, progress );
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
    int elapsed_succ = Time::passedSince( lastsucces_ );
    int elapsed_atmpt = Time::passedSince( timestamp_ );

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
	BufferString dbmsg("JobCommunic::sendMsg -- sending : ");
	dbmsg += statstr;		
	DBG::message(dbmsg);
    }

    char tagstr[3];
    tagstr[0] = tag;
    tagstr[1] = statstr.sepChar();
    tagstr[2] = '\0';
    BufferString buf( tagstr );
    buf += statstr;
    socket_->write( buf );

    char masterinfo;
    BufferString errbuf;
    BufferString inp;
    socket_->waitForReadyRead( 2000 );
    socket_->read( inp ); 
    masterinfo = inp[0];
    bool ret = !inp.isEmpty();
    if ( !ret )
    {
	BufferString emsg( "Error writing status to Master: " );
	emsg += errbuf;
	setErrMsg( errbuf );
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
	BufferString emsg( "Master sent an unkown response code. " );
	emsg += errbuf;
	setErrMsg( errbuf );
	ret = false;
    }

    return ret;
}


void JobCommunic::checkMasterTimeout()
{
    int elapsed = Time::passedSince( lastsucces_ );  

    if ( elapsed > 0 && elapsed > failtimeout_ )
    {
	BufferString msg( "Time-out contacting master." );
	msg += " Last contact "; msg += elapsed/1000;
	msg += " sec ago. Exiting.";
	directMsg( msg );
	ExitProgram( -1 );
    }
}


void JobCommunic::directMsg( const char* msg )
{
    (sdout_.ostrm ? *sdout_.ostrm : std::cerr) << msg << std::endl;
}


void JobCommunic::alarmHndl(CallBacker*)
{
    // no need to do anything - see comments at JobIOHandler::alarmHndl
    UsrMsg( "MM Socket Communication: time-out." );
}


void JobCommunic::disConnect()
{
    socket_->disconnectFromHost();
}
