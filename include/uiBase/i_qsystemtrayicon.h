#ifndef i_qsystemtrayicon_h
#define i_qsystemtrayicon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: i_qsystemtrayicon.h,v 1.1 2010-09-20 06:07:45 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisystemtrayicon.h"

#include <QSystemTrayIcon> 

//! Helper class for uiSystemTrayIcon to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

class QSystemTrayIconMessenger : public QObject 
{
Q_OBJECT
friend class uiSystemTrayIcon;

protected:

QSystemTrayIconMessenger( QSystemTrayIcon* sender, uiSystemTrayIcon* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	     this, SLOT(activated(QSystemTrayIcon::ActivationReason)) );
    connect( sender, SIGNAL(messageClicked()), this, SLOT(messageClicked()) );
}


private:

    uiSystemTrayIcon* 	receiver_;
    QSystemTrayIcon*  	sender_;


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

#endif
