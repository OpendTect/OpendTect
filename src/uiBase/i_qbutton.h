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
    friend class	uiButton;

public:

i_ButMessenger( QAbstractButton& sndr, uiButtonMessenger& receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
#define mConnectButMsngr(nm,args) \
    connect( &sender_, SIGNAL(nm(args)), this, SLOT(nm(args)) )

    mConnectButMsngr( toggled, bool );
    mConnectButMsngr( clicked, );
    mConnectButMsngr( pressed, );
    mConnectButMsngr( released, );
}

private:

    uiButtonMessenger&	receiver_;
    QAbstractButton&	sender_;

public slots:

    void toggled(bool)	{ receiver_.notifyHandler(uiButtonMessenger::toggled); }
    void clicked()	{ receiver_.notifyHandler(uiButtonMessenger::clicked); }
    void pressed()	{ receiver_.notifyHandler(uiButtonMessenger::pressed); }
    void released()	{ receiver_.notifyHandler(uiButtonMessenger::released);}

};


class i_ButtonGroupMessenger : public QObject
{

    Q_OBJECT
    friend class	uiButtonGroup;

public:

i_ButtonGroupMessenger( QButtonGroup& sndr, uiButtonGroup& receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
    connect( &sender_, SIGNAL(buttonClicked(QAbstractButton*)),
	    this, SLOT(clicked()) );
}

private:

    uiButtonGroup&	receiver_;
    QButtonGroup&	sender_;

public slots:

    void clicked()      { receiver_.valueChanged.trigger(); }
};

QT_END_NAMESPACE
