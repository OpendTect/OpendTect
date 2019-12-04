#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "ptrman.h"

namespace DBG { mGlobal(Basic) void forceCrash(bool); }

/*!
\brief Asynchronous event handling and notification.
*/

mExpClass(Basic) SignalHandling : public CallBacker
{
public:

    static void			initClass();

    enum EvType			{
				    ConnClose,
				    ChldStop,
				    ReInit,
				    Stop,
				    Cont,
				    Alarm,
				    Kill	// This process
				};

    static void			startNotify(EvType,const CallBack&);
    static void			stopNotify(EvType,const CallBack&);
    static void			stopProcess(PID_Type,bool friendly=true);
    static void			stopRemote(const char*,PID_Type,
					    bool friendly=true,
					    const char* rshcomm=0 );
    static void			initFatalSignalHandling();

protected:

				SignalHandling();
				~SignalHandling();
    static SignalHandling&	SH();

    CallBackSet&		conncbs_;
    CallBackSet&		chldcbs_;
    CallBackSet&		reinitcbs_;
    CallBackSet&		stopcbs_;
    CallBackSet&		contcbs_;
    CallBackSet&		alarmcbs_;
    CallBackSet&		killcbs_;

    CallBackSet&		getCBL(EvType);

    static void			handle(int);

    void			doKill(PID_Type);
    void			doStop(PID_Type,bool withcbs=true);
    void			doCont();
    void			handleConn();
    void			handleChld();
    void			handleAlarm();
    void			handleReInit();


    friend void			DBG::forceCrash(bool);

};
