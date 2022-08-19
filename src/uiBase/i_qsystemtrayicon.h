#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisystemtrayicon.h"

#include <QSystemTrayIcon>

//! Helper class for uiSystemTrayIcon to relay Qt's messages.
/*! Internal object, to hide Qt's signal/slot mechanism. */

QT_BEGIN_NAMESPACE

class QSystemTrayIconMessenger : public QObject
{
Q_OBJECT
friend class uiSystemTrayIcon;

protected:

QSystemTrayIconMessenger( QSystemTrayIcon* sndr, uiSystemTrayIcon* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	     this, SLOT(activated(QSystemTrayIcon::ActivationReason)) );
    connect( sndr, SIGNAL(messageClicked()), this, SLOT(messageClicked()) );
}


private:

    uiSystemTrayIcon*	receiver_;
    QSystemTrayIcon*	sender_;


private slots:

void messageClicked()
{ receiver_->messageClicked.trigger( *receiver_ ); }

void activated( QSystemTrayIcon::ActivationReason reason )
{
    if ( reason == QSystemTrayIcon::Context )
	receiver_->rightClicked.trigger( *receiver_ );
    else if ( reason == QSystemTrayIcon::DoubleClick )
	receiver_->doubleClicked.trigger( *receiver_ );
    else if ( reason == QSystemTrayIcon::Trigger )
	receiver_->clicked.trigger( *receiver_ );
    else if ( reason == QSystemTrayIcon::MiddleClick )
	receiver_->middleClicked.trigger( *receiver_ );
}

};

QT_END_NAMESPACE
