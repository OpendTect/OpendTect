#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilineedit.h"

#include <QLineEdit>

QT_BEGIN_NAMESPACE


//! Helper class for uilineedit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_lineEditMessenger : public QObject
{
Q_OBJECT
friend class uiLineEditBody;

protected:
i_lineEditMessenger( QLineEdit* sndr, uiLineEdit* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QLineEdit::returnPressed,
	     this, &i_lineEditMessenger::returnPressed );
    connect( sndr, &QLineEdit::editingFinished,
	     this, &i_lineEditMessenger::editingFinished );
    connect( sndr, &QLineEdit::textChanged,
	     this, &i_lineEditMessenger::textChanged );
    connect( sndr, &QLineEdit::selectionChanged,
	     this, &i_lineEditMessenger::selectionChanged );
}


~i_lineEditMessenger()
{}

private:

    QLineEdit*	sender_;
    uiLineEdit*	receiver_;

private slots:

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

void editingFinished()
{
    if ( !sender_->isModified() )
	return;

    mTrigger( editingFinished );
}

void returnPressed()
{
    mTrigger( returnPressed );
}

void textChanged(const QString&)
{
    mTrigger( textChanged );
}

void selectionChanged()
{
    receiver_->selectionChanged.trigger(*receiver_);
}

#undef mTrigger
};

QT_END_NAMESPACE
