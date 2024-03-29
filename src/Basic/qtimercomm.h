#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    connect( qtimer, &QTimer::timeout, this, &QTimerComm::timeout );
}

public:

~QTimerComm()
{
    deactivate();
}


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
