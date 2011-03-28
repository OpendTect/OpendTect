#ifndef i_qaction_h
#define i_qaction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: i_qaction.h,v 1.4 2011-03-28 08:46:04 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiaction.h"

#include <QObject>
#include <QAction>
#include <iostream>

//! Helper class for uiAction to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_ActionMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiAction;

protected:
i_ActionMessenger( QAction* sender, uiAction* receiver )
    : sender_( sender )
    , receiver_( receiver )
{ 
    connect( sender, SIGNAL(toggled(bool)),this, SLOT(toggled(bool)) );
    connect( sender, SIGNAL(triggered(bool)), this, SLOT(triggered(bool)));
    connect( sender, SIGNAL(hovered()), this, SLOT(hovered()) );
}

virtual	~i_ActionMessenger() {}
   
private:

    uiAction*		receiver_;
    QAction*		sender_;

private slots:

void toggled( bool checked )
{
    receiver_->checked_ = checked;
    receiver_->toggled.trigger( *receiver_ );
}

void triggered( bool checked )
{
    receiver_->checked_ = checked;
    receiver_->triggered.trigger( *receiver_ );
}


void hovered()
{
}

};

#endif
