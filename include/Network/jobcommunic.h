#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    mDeprecatedDef	JobCommunic(const char* host,PortNr_Type,
				    int jobid,StreamData&);
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

protected:

// TODO: Rename to primaryauth_;
    Network::Authority	masterauth_;
    bool		stillok_ = true;
    State		stat_;
    uiString		errmsg_;
    int			jobid_;
    bool		pausereq_ = false;
    od_ostream*		strm_ = nullptr;

    Network::Socket*	socket_;

    bool		sendState_(State,bool isexit,bool immediate);
    bool		sendProgress_(int,bool immediate);
    bool		sendPID_(PID_Type);
    bool		sendErrMsg_(const char* msg);

    void		alarmHndl(CallBacker*); //!< time-out

private:

    struct sendData
    {
			sendData(char tag,int stat,const char* msg);
			~sendData();

	char		tag_;
	int		status_;
	BufferString	msg_;
    };

    bool		updateMsg(char tag,int,const char* msg=nullptr);
    bool		sendMsg(char tag,int,const char* msg=nullptr);
    void		sendMsgCB(CallBacker* cb=nullptr);
    BufferString	buildString(char tag,int,const char* msg=nullptr);

			//! directly to bp.stdout.ostrem or std::cerr.
    void		directMsg( const char* msg );

    void		setErrMsg(const char*);

    Network::Authority& primaryAuthority();
    void		checkPrimaryHostTimeout();

    int			timestamp_;
    int			nrattempts_ = 0;
    int			maxtries_;
    int			socktimeout_;
    int			failtimeout_;
    int			min_time_between_update_;
    int			lastsucces_;
    int			min_time_between_msgupdates_;
    int			lastupdate_;

    void		logMsg(bool stat,const char* msg,const char* details);
    bool		sendret_ = false;
    Threads::Lock	lock_;
    Threads::Lock	sendmsglock_;
    od_ostream*		logstream_;
    od_ostream*		createLogStream();
    void		dumpSystemInfo();

};
