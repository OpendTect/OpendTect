#ifndef i_qdial_h
#define i_qdial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: i_qdial.h,v 1.3 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidial.h"

#include <QObject>
#include <QDial> 

//! Helper class for uidial to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_DialMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiDialBody;

protected:

i_DialMessenger( QDial* sndr, uiDial* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{ 
    connect( sndr, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)) );
    connect( sndr, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
    connect( sndr, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    connect( sndr, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)) );
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
