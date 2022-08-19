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
    friend class	uiComboBoxBody;

protected:
			i_comboMessenger( QComboBox*  sndr,
					 uiComboBox* receiver )
			: _sender( sndr )
			, _receiver( receiver )
			{
			    connect( sndr, SIGNAL( activated (int)),
				     this,   SLOT( activated (int)) );

			    connect( sndr,
				     SIGNAL( editTextChanged(const QString&)),
				     this,
				     SLOT( editTextChanged(const QString&)) );
			}

    virtual		~i_comboMessenger() {}

private:

    uiComboBox*	_receiver;
    QComboBox*	_sender;

private slots:

/*!
    Handler for activated events.
    \sa QComboBox::activated
*/

void activated( int )
{ _receiver->notifyHandler( true ); }


void editTextChanged( const QString& )
{ _receiver->notifyHandler( false ); }


};

QT_END_NAMESPACE
