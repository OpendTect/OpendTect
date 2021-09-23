#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		9-5-2005
 RCS:		$Id$
________________________________________________________________________

*/

#include "networkcommon.h"

#include "applicationdata.h"
#include "genc.h"
#include "od_ostream.h"

class BatchProgram;
class StreamData;
namespace Network { class Socket; }

#define mReturn( ret ) { \
    if ( ret ) { nrattempts_ = 0; return true; } \
    if ( nrattempts_++ < maxtries_ ) return true; \
    stillok_ = false; \
    directMsg("Lost connection with primary host[1]. Exiting."); \
    ApplicationData::exit( -1 ); return false; \
}

#define mTryMaxtries( fn ) { \
    for ( int i=0; i<maxtries_; i++ ) \
    { \
	bool ret = fn; \
	if ( ret ) return true; \
	sleepSeconds(1); \
    } \
    stillok_ = false; \
    directMsg("Lost connection with primary host[2]. Exiting."); \
    ApplicationData::exit( -1 ); return false; \
}


/*! \brief Multi-machine socket communicator
 *  Handles the communication between a client and the primary host, from
 *  the client's point of view.
 */
mExpClass(Network) JobCommunic : public CallBacker
{ mODTextTranslationClass(JobCommunic);
public:
    enum State		{ Undef, Working, WrapUp, Finished, AllDone, Paused,
			  JobError, HostError, Killed, Timeout };

			JobCommunic(const char* host,PortNr_Type,
				    int jobid);
    mDeprecatedDef	JobCommunic(const char* host,PortNr_Type,
				    int jobid,StreamData&);
			~JobCommunic();

    bool		ok()		{ return stillok_; }
    uiString		errMsg()	{ return errmsg_; }

    State		state()	const	{ return stat_; }
    void		setState( State s ) { stat_ = s; }

    void		setStream( od_ostream& strm ) { strm_ = &strm; }

    bool		updateState()
			{
			    bool ret = sendState_(stat_,false,false);
			    mReturn(ret)
			}
    bool		updateProgress( int p )
			{ bool ret = sendProgress_(p,false); mReturn(ret) }

    void		setTimeBetweenMsgUpdates(int);

    bool		sendState(  bool isexit=false )
			    { mTryMaxtries( sendState_(stat_,isexit,true) ) }
    bool		sendProgress( int p )
			    { mTryMaxtries( sendProgress_(p,true) ) }

			//! hostrelated error messages are more serious.
    bool		sendErrMsg( const char* msg )
			    { mTryMaxtries( sendErrMsg_(msg) ) }
    bool		sendPID( int pid )
			    { mTryMaxtries( sendPID_(pid) ) }

    bool		pauseRequested() const
			    { return pausereq_; }
    void		disConnect();
protected:

// TODO: Rename to primaryauth_;
    Network::Authority	masterauth_;
    bool		stillok_;
    State		stat_;
    uiString		errmsg_;
    int			jobid_;
    bool		pausereq_;
    od_ostream*		strm_ = nullptr;

    Network::Socket*	socket_;

    bool		sendState_( State, bool isexit, bool immediate );
    bool		sendProgress_( int, bool immediate );
    bool		sendPID_( int );
    bool		sendErrMsg_( const char* msg );

    void		alarmHndl( CallBacker* ); //!< time-out

private:

    void		sendMsgCB(CallBacker* cb=nullptr);
    bool		updateMsg( char tag, int, const char* msg=0 );
    bool		sendMsg( char tag, int, const char* msg=0 );
    BufferString	buildString(char tag, int, const char* msg=0 );

			//! directly to bp.stdout.ostrem or std::cerr.
    void		directMsg( const char* msg );

    void		setErrMsg(const char*);

    Network::Authority& primaryAuthority();
    void		checkPrimaryHostTimeout();

    int			timestamp_;
    int			nrattempts_;
    int			maxtries_;
    int			socktimeout_;
    int			failtimeout_;
    int			min_time_between_update_;
    int			lastsucces_;
    int			min_time_between_msgupdates_;
    int			lastupdate_;

    void		logMsg(bool stat,const char* msg, const char* details);
    od_ostream*		logstream_;
    od_ostream*		createLogStream();
    void		dumpSystemInfo();
    void		clearHiddenParams();

    mDeprecated("Use checkPrimaryHostTimeout()")
    void		checkMasterTimeout();
};

#undef mReturn
#undef mTryMaxtries

