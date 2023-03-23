#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolbar.h"

#include <QToolBar>


//! Helper class for uiToolBar to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_ToolBarMessenger : public QObject
{
Q_OBJECT
friend class uiToolBar;

protected:
i_ToolBarMessenger( QToolBar* sndr, uiToolBar* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QToolBar::actionTriggered,
	     this, &i_ToolBarMessenger::actionTriggered );
    connect( sndr, &QToolBar::orientationChanged,
	     this, &i_ToolBarMessenger::orientationChanged );
}


~i_ToolBarMessenger()
{}

private:
    QToolBar*		sender_;
    uiToolBar*		receiver_;

private slots:

void actionTriggered( QAction* qaction )
{
    const int butid = receiver_->getID( qaction );
    receiver_->buttonClicked.trigger( butid, *receiver_ );
}

void orientationChanged( Qt::Orientation )
{
    receiver_->orientationChanged.trigger( *receiver_ );
}

};

QT_END_NAMESPACE
