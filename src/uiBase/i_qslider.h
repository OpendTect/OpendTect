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

i_SliderMessenger( QSlider* sndr, uiSlider* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)) );
    connect( sndr, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
    connect( sndr, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    connect( sndr, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)) );
}


private:

    uiSlider*	receiver_;
    QSlider*	sender_;

#define mTrigger( notifier ) \
    const int refnr = receiver_->slider()->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->slider()->endCmdRecEvent( refnr, #notifier );

private slots:

void sliderMoved(int)	{ mTrigger(sliderMoved); }
void sliderPressed()	{ mTrigger(sliderPressed); }
void sliderReleased()	{ mTrigger(sliderReleased); }
void valueChanged(int)	{ mTrigger(valueChanged); }

#undef mTrigger

};

QT_END_NAMESPACE
