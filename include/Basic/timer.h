#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/10/2000
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "notify.h"

mFDQtclass(QTimer)
mFDQtclass(QTimerComm)

/*!
\brief Timer class.
*/

mExpClass(Basic) Timer : public NamedCallBacker
{
public:

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

    mQtclass(QTimer*)		timer_;
    mQtclass(QTimerComm*)	comm_;
    ScriptPolicy		scriptpolicy_;

public:
				// Not for casual use

    ScriptPolicy		scriptPolicy() const;
    static bool			setUserWaitFlag(bool);	// returns old state

    static Notifier<Timer>*	timerStarts();
    static Notifier<Timer>*	timerStopped();
    static Notifier<Timer>*	timerShoots();
    static Notifier<Timer>*	timerShot();

};
