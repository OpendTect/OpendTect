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
    friend class	uiLineEditBody;

protected:
i_lineEditMessenger( QLineEdit* sndr, uiLineEdit* receiver )
    : sender_( sndr )
    , receiver_( receiver )
{
    connect( sndr, SIGNAL(returnPressed()),
	     this, SLOT(returnPressed()) );
    connect( sndr, SIGNAL(editingFinished()),
	     this, SLOT(editingFinished()) );
    connect( sndr, SIGNAL(textChanged(const QString&)),
	     this, SLOT(textChanged(const QString&)) );
    connect( sndr, SIGNAL(selectionChanged()),
	     this, SLOT(selectionChanged()) );
}

private:

    uiLineEdit*	receiver_;
    QLineEdit*	sender_;

private slots:

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

    void		editingFinished()
			{
			    if ( !sender_->isModified() )
				return;

			    mTrigger( editingFinished );
			}

    void		returnPressed()
			{ mTrigger( returnPressed ); }
    void		textChanged(const QString&)
			{ mTrigger( textChanged ); }

    void		selectionChanged()
			{ receiver_->selectionChanged.trigger(*receiver_); }
#undef mTrigger
};

QT_END_NAMESPACE
