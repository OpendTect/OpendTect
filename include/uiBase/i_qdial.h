#ifndef i_qdial_h
#define i_qdial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qdial.h,v 1.1 2010-01-13 08:12:40 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidial.h"

#include <QObject>
#include <QDial> 

class QString;

//! Helper class for uidial to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_DialMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiDialBody;

protected:

i_DialMessenger( QDial* sender, uiDial* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)) );
    connect( sender, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
    connect( sender, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    connect( sender, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)) );
}


private:

    uiDial* 	receiver_;
    QDial*  	sender_;

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

private slots:

void sliderMoved(int)	{ mTrigger(sliderMoved); }
void sliderPressed()	{ mTrigger(sliderPressed); }
void sliderReleased()	{ mTrigger(sliderReleased); }
void valueChanged(int)	{ mTrigger(valueChanged); }

};

#endif
