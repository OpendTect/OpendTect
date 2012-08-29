#ifndef i_qaction_h
#define i_qaction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: i_qaction.h,v 1.7 2012-08-29 16:21:06 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiaction.h"

#include <QObject>
#include <QAction>
#include <iostream>

//! Helper class for uiAction to relay Qt's messages.

QT_BEGIN_NAMESPACE

class i_ActionMessenger : public QObject
{
    Q_OBJECT
    friend class	uiAction;

protected:
i_ActionMessenger( QAction* sndr, uiAction* receiver )
    : sender_( sndr )
    , receiver_( receiver )
{ 
    connect( sender_, SIGNAL(toggled(bool)),this, SLOT(toggled(bool)) );
    connect( sender_, SIGNAL(triggered(bool)), this, SLOT(triggered(bool)));
    connect( sender_, SIGNAL(hovered()), this, SLOT(hovered()) );
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

QT_END_NAMESPACE

#endif
