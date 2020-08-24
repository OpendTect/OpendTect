#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/

#include "uispinbox.h"

#include <QDoubleSpinBox>

//! Helper class for uiSpinBox to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_SpinBoxMessenger : public QObject
{
    Q_OBJECT
    friend class uiSpinBoxBody;

protected:
			i_SpinBoxMessenger(QDoubleSpinBox*  sndr,
					   uiSpinBox* receiver)
			: sender_(sndr)
			, receiver_(receiver)
			{
			    connect( sndr, SIGNAL(editingFinished()),
				     this, SLOT(editingFinished()) );
			    connect(sndr, SIGNAL(valueChanged(double)),
				    this, SLOT(valueChanged(double)) );
			}

    virtual		~i_SpinBoxMessenger() {}

private:

    uiSpinBox*		receiver_;
    QDoubleSpinBox*	sender_;

private slots:

    void		editingFinished()
			{ receiver_->notifyHandler( true ); }
    void		valueChanged(double)
			{ receiver_->notifyHandler( false ); }
};

QT_END_NAMESPACE
