#ifndef i_qslider_h
#define i_qslider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qslider.h,v 1.6 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uislider.h"

#include <QObject>
#include <QSlider> 

class QString;

//! Helper class for uislider to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_SliderMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiSliderBody;

protected:

i_SliderMessenger( QSlider* sender, uiSlider* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)) );
    connect( sender, SIGNAL(sliderPressed()), this, SLOT(sliderPressed()) );
    connect( sender, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    connect( sender, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)) );
}


private:

    uiSlider* 	receiver_;
    QSlider*  	sender_;

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
