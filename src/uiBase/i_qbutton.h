#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibutton.h"
#include "uibuttongroup.h"

#include <QAbstractButton>
#include <QButtonGroup>

/*! Help class, because templates can not use signals/slots
    Relays QT button signals to the notifyHandler of a uiButton object.
*/

QT_BEGIN_NAMESPACE

class i_ButMessenger : public QObject
{
Q_OBJECT
friend class uiButton;

public:

i_ButMessenger( QAbstractButton& sndr, uiButtonMessenger& rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( &sender_, &QAbstractButton::toggled,
	     this, &i_ButMessenger::toggled );
    connect( &sender_, &QAbstractButton::clicked,
	     this, &i_ButMessenger::clicked );
    connect( &sender_, &QAbstractButton::pressed,
	     this, &i_ButMessenger::pressed );
    connect( &sender_, &QAbstractButton::released,
	     this, &i_ButMessenger::released );
}


~i_ButMessenger()
{}


private:

    QAbstractButton&	sender_;
    uiButtonMessenger&	receiver_;

public slots:

void toggled(bool)
{
    receiver_.notifyHandler( uiButtonMessenger::toggled );
}

void clicked()
{
    receiver_.notifyHandler( uiButtonMessenger::clicked );
}

void pressed()
{
    receiver_.notifyHandler( uiButtonMessenger::pressed );
}

void released()
{
    receiver_.notifyHandler( uiButtonMessenger::released );
}

};


class i_ButtonGroupMessenger : public QObject
{
Q_OBJECT
friend class uiButtonGroup;

public:

i_ButtonGroupMessenger( QButtonGroup& sndr, uiButtonGroup& receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
    connect( &sender_,
	     QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
	     this, &i_ButtonGroupMessenger::clicked );
}


~i_ButtonGroupMessenger()
{}

private:

    uiButtonGroup&	receiver_;
    QButtonGroup&	sender_;

public slots:

void clicked()
{
    receiver_.valueChanged.trigger();
}

};

QT_END_NAMESPACE
