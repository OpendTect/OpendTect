#ifndef sighndl_h
#define sighndl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "ptrman.h"

namespace DBG { mGlobal(Basic) void forceCrash(bool); }

/*!\brief asynchronous event handling and notification. */


mClass(Basic) SignalHandling : public CallBacker
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
    static void			stopProcess(int,bool friendly=true);
    static void			stopRemote( const char*,int, bool friendly=true,
					    const char* rshcomm=0 );
    static void			initFatalSignalHandling();

protected:

					SignalHandling();
    static SignalHandling&		SH();
    					/*!<Access to a static instance */

    CallBackSet				conncbs_;
    CallBackSet				chldcbs_;
    CallBackSet				reinitcbs_;
    CallBackSet				stopcbs_;
    CallBackSet				contcbs_;
    CallBackSet				alarmcbs_;
    CallBackSet				killcbs_;

    CallBackSet&			getCBL(EvType);

    static void				handle(int);

    void				doKill(int);
    void				doStop(int,bool withcbs=true);
    void				doCont();
    void				handleConn();
    void				handleChld();
    void				handleAlarm();
    void				handleReInit();


    friend void				DBG::forceCrash(bool);

};


#endif

