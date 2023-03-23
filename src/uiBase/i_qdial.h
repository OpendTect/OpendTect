#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidial.h"

#include <QDial>

//! Helper class for uidial to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_DialMessenger : public QObject
{
Q_OBJECT
friend class uiDialBody;

protected:

i_DialMessenger( QDial* sndr, uiDial* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QAbstractSlider::sliderMoved,
	     this, &i_DialMessenger::sliderMoved );
    connect( sndr, &QAbstractSlider::sliderPressed,
	     this, &i_DialMessenger::sliderPressed );
    connect( sndr, &QAbstractSlider::sliderReleased,
	     this, &i_DialMessenger::sliderReleased );
    connect( sndr, &QAbstractSlider::valueChanged,
	     this, &i_DialMessenger::valueChanged );
}


~i_DialMessenger()
{}


private:

    QDial*	sender_;
    uiDial*	receiver_;

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

private slots:

void sliderMoved(int)
{
    mTrigger(sliderMoved);
}

void sliderPressed()
{
    mTrigger(sliderPressed);
}

void sliderReleased()
{
    mTrigger(sliderReleased);
}

void valueChanged(int)
{
    mTrigger(valueChanged);
}

#undef mTrigger
};

QT_END_NAMESPACE
