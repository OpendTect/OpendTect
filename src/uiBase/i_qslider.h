#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uislider.h"

#include <QSlider>

//! Helper class for uislider to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class QString;

class i_SliderMessenger : public QObject
{
Q_OBJECT
friend class	uiSliderBody;

protected:

i_SliderMessenger( QSlider* sndr, uiSlider* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QAbstractSlider::sliderMoved,
	     this, &i_SliderMessenger::sliderMoved );
    connect( sndr, &QAbstractSlider::sliderPressed,
	     this, &i_SliderMessenger::sliderPressed );
    connect( sndr, &QAbstractSlider::sliderReleased,
	     this, &i_SliderMessenger::sliderReleased );
    connect( sndr, &QAbstractSlider::valueChanged,
	     this, &i_SliderMessenger::valueChanged );
}


~i_SliderMessenger()
{}


private:

    QSlider*	sender_;
    uiSlider*	receiver_;

#define mTrigger( notifier ) \
    const int refnr = receiver_->slider()->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->slider()->endCmdRecEvent( refnr, #notifier );

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
