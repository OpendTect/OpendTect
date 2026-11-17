#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiaction.h"

#include <QAction>
#include <iostream>

//! Helper class for uiAction to relay Qt's messages.

QT_BEGIN_NAMESPACE

class i_ActionMessenger : public QObject
{
Q_OBJECT
friend class uiAction;

protected:
i_ActionMessenger( QAction* sndr, uiAction* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sender_, &QAction::toggled,
	     this, &i_ActionMessenger::toggled );
    connect( sender_, &QAction::triggered,
	     this, &i_ActionMessenger::triggered);
    connect( sender_, &QAction::hovered,
	     this, &i_ActionMessenger::hovered );
}

~i_ActionMessenger()
{}

private:

    QAction*		sender_;
    uiAction*		receiver_;

private slots:

void toggled( bool checked )
{
    receiver_->checked_ = checked;
    receiver_->toggled.trigger( *receiver_ );
}

void triggered( bool checked )
{
    const int refnr = receiver_->beginCmdRecEvent();
    receiver_->trigger( checked );
    receiver_->endCmdRecEvent( refnr );
}


void hovered()
{
}

};

QT_END_NAMESPACE
