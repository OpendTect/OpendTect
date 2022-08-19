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
    friend class	uiToolBar;

protected:
i_ToolBarMessenger( QToolBar* sndr, uiToolBar* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(actionTriggered(QAction*)),
	     this, SLOT(actionTriggered(QAction*)) );
    connect( sndr, SIGNAL(orientationChanged(Qt::Orientation)),
	     this, SLOT(orientationChanged(Qt::Orientation)) );
}

private:
    uiToolBar*		receiver_;
    QToolBar*		sender_;

private slots:

void actionTriggered( QAction* qaction )
{
    const int butid = receiver_->getID( qaction );
    receiver_->buttonClicked.trigger( butid, *receiver_ );
}

void orientationChanged( Qt::Orientation )
{ receiver_->orientationChanged.trigger( *receiver_ ); }

};

QT_END_NAMESPACE
