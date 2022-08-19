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
    friend class	uiDialBody;

protected:

i_DialMessenger( QDial* sndr, uiDial* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)) );
    connect( sndr, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
    connect( sndr, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    connect( sndr, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)) );
}


private:

    uiDial*	receiver_;
    QDial*	sender_;

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

private slots:

void sliderMoved(int)	{ mTrigger(sliderMoved); }
void sliderPressed()	{ mTrigger(sliderPressed); }
void sliderReleased()	{ mTrigger(sliderReleased); }
void valueChanged(int)	{ mTrigger(valueChanged); }

#undef mTrigger

};

QT_END_NAMESPACE
