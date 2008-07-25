#ifndef i_qslider_h
#define i_qslider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: i_qslider.h,v 1.4 2008-07-25 07:08:01 cvsnanne Exp $
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

private slots:

void sliderMoved(int)	{ receiver_->sliderMoved.trigger(receiver_); }
void sliderPressed()	{ receiver_->sliderPressed.trigger(receiver_); }
void sliderReleased()	{ receiver_->sliderReleased.trigger(receiver_); }
void valueChanged(int)	{ receiver_->valueChanged.trigger(receiver_); }

};

#endif
