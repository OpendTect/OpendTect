#ifndef timer_h
#define timer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/10/2000
 RCS:           $Id: timer.h,v 1.2 2009-07-22 16:01:17 cvsbert Exp $
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
