/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.2 2001-05-04 10:09:03 windev Exp $
________________________________________________________________________

-*/

#include "uislider.h"

#include "i_qslider.h"
#include "i_qobjwrap.h"

#include <qsize.h> 

//------------------------------------------------------------------------------

uiSlider::uiSlider(  uiObject* parnt, const char* nm )
	: uiWrapObj<i_QSlider>(new i_QSlider(*this,parnt), parnt, nm)
	, _messenger ( *new i_SliderMessenger( mQtThing(), this ))
	, valueChanged(this)
	, sliderMoved(this)
{
    mQtThing()->setOrientation( QSlider::Horizontal );
    setTickMarks( true );
}

const QWidget* 	uiSlider::qWidget_() const 	{ return mQtThing(); } 

const char* uiSlider::text() const
{
    result = mQtThing()->value();
    return (const char*)result;
}


int uiSlider::getIntValue() const
{
    return mQtThing()->value();
}


double uiSlider::getValue() const
{
    return mQtThing()->value();
}


void uiSlider::setText( const char* t )
{
    setValue( atoi(t) );
}


void uiSlider::setValue( int i )
{
    mQtThing()->setValue( i );
}


void uiSlider::setValue( double d )
{
    mQtThing()->setValue( d );
}

void uiSlider::setTickMarks( bool yn )
{
    mQtThing()->setTickmarks ( yn ? QSlider::Below : QSlider::NoMarks );
}
