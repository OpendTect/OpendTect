#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicombobox.h"

#include <QComboBox>

//! Helper class for uiComboBox to relay Qt's 'activated' messages to uiAction.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_comboMessenger : public QObject
{
Q_OBJECT
friend class uiComboBoxBody;

protected:
i_comboMessenger( QComboBox* sndr, uiComboBox* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sender_, QOverload<int>::of(&QComboBox::activated),
	     this, &i_comboMessenger::activated );
    connect( sender_, &QComboBox::editTextChanged,
	     this, &i_comboMessenger::editTextChanged );
}


~i_comboMessenger()
{}

private:

    QComboBox*	sender_;
    uiComboBox*	receiver_;

private slots:

void activated( int )
{
    receiver_->notifyHandler( true );
}


void editTextChanged( const QString& )
{
    receiver_->notifyHandler( false );
}

};

QT_END_NAMESPACE
