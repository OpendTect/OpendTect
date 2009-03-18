#ifndef qtimercomm_h
#define qtimercomm_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: qtimercomm.h,v 1.1 2009-03-18 04:24:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include <QTimer>
#include "timer.h"

/*\brief QTimer communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

class QTimerComm : public QObject 
{
    Q_OBJECT
    friend class	Timer;

protected:

QTimerComm( QTimer* qtimer, Timer* timer )
    : qtimer_(qtimer)
    , timer_(timer)
    , magic_( 0xdeadbeef )
{
    connect( qtimer, SIGNAL(timeout()), this, SLOT(timeout()) );
}

public:

virtual	~QTimerComm()
{ deactivate(); }


void deactivate() 
{
    if ( qtimer_ && magic_ == 0xdeadbeef ) 
	qtimer_->stop();

    qtimer_ = 0;
    timer_ = 0;
    magic_ = 0;
}

private slots:

void timeout()
{ 
    if ( timer_ && magic_ == 0xdeadbeef ) 
	timer_->tick.trigger( *timer_ );
}

private:

    QTimer*		qtimer_;
    Timer*		timer_;
    int 		magic_;

};

#endif
