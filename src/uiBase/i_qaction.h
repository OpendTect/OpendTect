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
    friend class	uiAction;

protected:
i_ActionMessenger( QAction* sndr, uiAction* receiver )
    : sender_( sndr )
    , receiver_( receiver )
{
    connect( sender_, SIGNAL(toggled(bool)),this, SLOT(toggled(bool)) );
    connect( sender_, SIGNAL(triggered(bool)), this, SLOT(triggered(bool)));
    connect( sender_, SIGNAL(hovered()), this, SLOT(hovered()) );
}

virtual	~i_ActionMessenger() {}

private:

    uiAction*		receiver_;
    QAction*		sender_;

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
