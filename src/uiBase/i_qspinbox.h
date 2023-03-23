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
i_SpinBoxMessenger( QDoubleSpinBox* sndr, uiSpinBox* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QAbstractSpinBox::editingFinished,
	     this, &i_SpinBoxMessenger::editingFinished );
    connect( sndr, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	     this, &i_SpinBoxMessenger::valueChanged );
}

~i_SpinBoxMessenger()
{}

private:

    QDoubleSpinBox*	sender_;
    uiSpinBox*		receiver_;

private slots:

void editingFinished()
{
    receiver_->notifyHandler( true );
}

void valueChanged(double)
{
    receiver_->notifyHandler( false );
}

};

QT_END_NAMESPACE
