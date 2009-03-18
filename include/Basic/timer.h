#ifndef timer_h
#define timer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/10/2000
 RCS:           $Id: timer.h,v 1.1 2009-03-18 04:24:39 cvsnanne Exp $
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

protected:
    QTimer*		timer_;
    QTimerComm*		comm_;
};


#endif
