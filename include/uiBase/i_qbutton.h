#ifndef i_qbutton_h
#define i_qbutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibutton.h"

#include <QAbstractButton>
#include <QObject>

/*! Help class, because templates can not use signals/slots
    Relays QT button signals to the notifyHandler of a uiButton object.
*/

class i_ButMessenger : public QObject 
{ 
    Q_OBJECT
    friend class	uiButton;
public:

i_ButMessenger( QAbstractButton* sndr, uiButtonBody* receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
    connect( sender_, SIGNAL(clicked()), this, SLOT(clicked()) );
    connect( sender_, SIGNAL(pressed()), this, SLOT(pressed()) );
    connect( sender_, SIGNAL(released()), this, SLOT(released()) );
    connect( sender_, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)) );
}

private:

    uiButtonBody*	receiver_;
    QAbstractButton*	sender_;

public slots:
void toggled(bool)	{ receiver_->notifyHandler( uiButtonBody::toggled ); }
void clicked()		{ receiver_->notifyHandler( uiButtonBody::clicked ); }
void pressed()		{ receiver_->notifyHandler( uiButtonBody::pressed ); }
void released()		{ receiver_->notifyHandler( uiButtonBody::released); }

};



#endif
