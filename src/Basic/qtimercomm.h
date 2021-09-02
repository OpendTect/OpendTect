#ifndef qtimercomm_h
#define qtimercomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/

#include <QTimer>
#include "timer.h"

QT_BEGIN_NAMESPACE

/*!
\brief QTimer communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

class QTimerComm : public QObject
{
    Q_OBJECT
    friend class	::Timer;

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

    qtimer_ = nullptr;
    timer_ = nullptr;
    magic_ = 0;
}

private slots:

void timeout()
{
    if ( timer_ && magic_ == 0xdeadbeef )
	timer_->notifyHandler();
}

private:

    QTimer*		qtimer_;
    Timer*		timer_;
    unsigned int 	magic_;

};

QT_END_NAMESPACE

#endif
