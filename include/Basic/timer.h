#ifndef timer_h
#define timer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/10/2000
 RCS:           $Id: timer.h,v 1.3 2009-12-04 14:36:42 cvsjaap Exp $
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
    void		start(int msec,bool singleshot=false);
    void		stop();

    Notifier<Timer>	tick;

    void		notifyHandler();

protected:
    QTimer*		timer_;
    QTimerComm*		comm_;

public:
			// Not for casual use
    static void		setTelltale(const CallBack&);
    static void		unsetTelltale();
    static int		nrFirstShotTimers();
};


#endif
