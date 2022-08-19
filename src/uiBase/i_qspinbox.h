#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
