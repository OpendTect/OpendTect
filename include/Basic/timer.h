#ifndef timer_h
#define timer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/10/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "namedobj.h"

class QTimer;
class QTimerComm;

mClass Timer : public NamedObject
{
public :
			Timer(const char* nm="Timer");
    virtual		~Timer();

    bool		isActive() const;
    bool		isSingleShot() const;

    void		start(int msec,bool singleshot=false);
    void		stop();

    Notifier<Timer>	tick;

    void		notifyHandler();

    enum ScriptPolicy	{ DefaultPolicy, DontWait, UserWait, KeepWaiting };
    void		setScriptPolicy(ScriptPolicy);

protected:
    QTimer*		timer_;
    QTimerComm*		comm_;
    ScriptPolicy	scriptpolicy_;		

public:	
				// Not for casual use

    ScriptPolicy		scriptPolicy() const;
    static bool			setUserWaitFlag(bool);	// returns old state

    static Notifier<Timer>*	timerStarts();
    static Notifier<Timer>*	timerStopped();
    static Notifier<Timer>*	timerShoots();
    static Notifier<Timer>*	timerShot();

};


#endif
