/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include <qsize.h> 


//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,QSlider>
{
public:

                        uiSliderBody( uiSlider& handle,
				      uiParent* parnt, const char* nm);

    virtual bool        isSingleLine() const { return true; }

private:

    i_SliderMessenger& messenger_;

};

uiSliderBody::uiSliderBody( uiSlider& handle,uiParent* parnt, const char* nm )
    : uiObjBodyImpl<uiSlider,QSlider>(handle, parnt, nm)
    , messenger_ ( *new i_SliderMessenger( this, &handle ))
{}


//------------------------------------------------------------------------------

uiSlider::uiSlider(  uiParent* parnt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt, nm) )
    , valueChanged(this)
    , sliderMoved(this)
{
    body_->setOrientation( QSlider::Horizontal );
    setTickMarks( true );
}

uiSliderBody& uiSlider::mkbody(uiParent* parnt, const char* nm)
{ 
    body_= new uiSliderBody(*this,parnt,nm);
    return *body_; 
}

const char* uiSlider::text() const
{
    result = body_->value();
    return (const char*)result;
}


int uiSlider::getIntValue() const
{
    return body_->value();
}


double uiSlider::getValue() const
{
    return body_->value();
}


void uiSlider::setText( const char* t )
{
    setValue( atoi(t) );
}


void uiSlider::setValue( int i )
{
    body_->setValue( i );
}


void uiSlider::setValue( double d )
{
    body_->setValue( d );
}

void uiSlider::setTickMarks( bool yn )
{
    body_->setTickmarks ( yn ? QSlider::Below : QSlider::NoMarks );
}
