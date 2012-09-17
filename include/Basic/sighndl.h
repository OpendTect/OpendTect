#ifndef sighndl_h
#define sighndl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: sighndl.h,v 1.13 2012/09/17 16:37:44 cvsjaap Exp $
________________________________________________________________________

-*/

#include "callback.h"

namespace DBG { mGlobal void forceCrash(bool); }

/*!\brief asynchronous event handling and notification. */


mClass SignalHandling : public CallBacker
{
public:

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
    static void			stopProcess(int,bool friendly=true);
    static void			stopRemote( const char*,int, bool friendly=true,
					    const char* rshcomm=0 );

protected:

				SignalHandling();
    static SignalHandling	theinst_;

    CallBackSet			conncbs;
    CallBackSet			chldcbs;
    CallBackSet			reinitcbs;
    CallBackSet			stopcbs;
    CallBackSet			contcbs;
    CallBackSet			alarmcbs;
    CallBackSet			killcbs;

    CallBackSet&		getCBL(EvType);

    static void			handle(int);

    void			doKill(int);
    void			doStop(int,bool withcbs=true);
    void			doCont();
    void			handleConn();
    void			handleChld();
    void			handleAlarm();
    void			handleReInit();


    friend void			DBG::forceCrash(bool);

public:
    static void			initFatalSignalHandling();
};


#endif
