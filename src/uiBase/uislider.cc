/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.9 2002-04-15 14:34:26 bert Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"

#include <qsize.h> 


//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,QSlider>
{
public:

                        uiSliderBody( uiSlider& handle,
				      uiParent* parnt, const char* nm);

    virtual int 	nrTxtLines() const				{ return 1; }

private:

    i_SliderMessenger& messenger_;

};

uiSliderBody::uiSliderBody( uiSlider& handle,uiParent* parnt, const char* nm )
    : uiObjBodyImpl<uiSlider,QSlider>(handle, parnt, nm)
    , messenger_ ( *new i_SliderMessenger( this, &handle ))
{
    setStretch( 1, 0 );
    setSzPol( SzPolicySpec().setHSzP( SzPolicySpec::medium) );
}


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
    body_->setValue( mNINT(d) );
}

void uiSlider::setTickMarks( bool yn )
{
    body_->setTickmarks ( yn ? QSlider::Above : QSlider::NoMarks );
}

int uiSlider::minValue() const		{ return body_->minValue() ; }
int uiSlider::maxValue() const		{ return body_->minValue() ; }
void uiSlider::setMinValue( int m )	{ body_->setMinValue(m); }
void uiSlider::setMaxValue( int m )	{ body_->setMaxValue(m); }
int uiSlider::tickStep() const     	{ return body_->tickInterval() ; }
void uiSlider::setTickStep ( int s )	{ body_->setTickInterval(s); }


uiLabeledSlider::uiLabeledSlider( uiParent* p, const char* txt,
				  const char* nm )
	: uiGroup(p,"Labeled slider")
{
    slider = new uiSlider( this, nm );
    lbl = new uiLabel( this, txt, slider );
    setHAlignObj( slider );
}
