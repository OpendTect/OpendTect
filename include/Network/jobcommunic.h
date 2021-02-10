#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		9-5-2005
________________________________________________________________________

*/

#include "networkcommon.h"

#include "genc.h"
#include "od_ostream.h"

class BatchProgram;
class StreamData;
namespace Network { class Socket; }


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
			~JobCommunic();

    bool		ok()		{ return stillok_; }
    uiString		errMsg()	{ return errmsg_; }

    State		state()	const	{ return stat_; }
    void		setState( State s ) { stat_ = s; }

    void		setStream( od_ostream& strm ) { strm_ = &strm; }

    bool		updateState();
    bool		updateProgress(int);

    void		setTimeBetweenMsgUpdates(int);

    bool		sendState(bool isexit = false);
    bool		sendProgress(int);

			//! hostrelated error messages are more serious.
    bool		sendErrMsg(const char*);
    bool		sendPID(PID_Type);

    bool		pauseRequested() const
			    { return pausereq_; }
    void		disConnect();

    void		setProgressDetail(const char* str)
			    { progressdetail_ = str; }

protected:

    Network::Authority	masterauth_;
    bool		stillok_ = true;
    State		stat_;
    uiString		errmsg_;
    int			jobid_;
    bool		pausereq_ = false;
    od_ostream*		strm_ = nullptr;
    BufferString	progressdetail_;

    Network::Socket*	socket_;

    bool		sendState_(State,bool isexit,bool immediate);
    bool		sendProgress_(int,bool immediate);
    bool		sendPID_(PID_Type);
    bool		sendErrMsg_(const char* msg);

    void		alarmHndl(CallBacker*); //!< time-out

private:

    bool		updateMsg( char tag, int, const char* msg=0 );
    bool		sendMsg( char tag, int, const char* msg=0 );

			//! directly to bp.stdout.ostrem or std::cerr.
    void		directMsg( const char* msg );

    void		setErrMsg(const char*);

    void		checkMasterTimeout();

    int			timestamp_;
    int			nrattempts_ = 0;
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

};
