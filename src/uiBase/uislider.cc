/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.10 2002-04-16 12:49:58 nanne Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "datainpspec.h"

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

uiSlider::uiSlider(  uiParent* parnt, const char* nm, bool witheditfld )
    : uiObject( parnt, nm, mkbody(parnt, nm) )
    , valueChanged(this)
    , sliderMoved(this)
    , editfld(0)
{
    body_->setOrientation( QSlider::Horizontal );
    setTickMarks( true );
    if ( witheditfld )
    {
	valueChanged.notify( mCB(this,uiSlider,sliderMove) );
	editfld = new uiLineEdit( parnt, FloatInpSpec() );
	editfld->attach( rightTo, this );
	editfld->returnPressed.notify( mCB(this,uiSlider,editValue) );
    }
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

void uiSlider::sliderMove( CallBacker* )
{
    if ( !editfld ) return;
    editfld->setValue( getIntValue() );
}

void uiSlider::editValue( CallBacker* )
{
    setValue( editfld->getValue() );
}

int uiSlider::minValue() const		{ return body_->minValue() ; }
int uiSlider::maxValue() const		{ return body_->minValue() ; }
void uiSlider::setMinValue( int m )	{ body_->setMinValue(m); }
void uiSlider::setMaxValue( int m )	{ body_->setMaxValue(m); }
int uiSlider::tickStep() const     	{ return body_->tickInterval() ; }
void uiSlider::setTickStep ( int s )	{ body_->setTickInterval(s); }


uiLabeledSlider::uiLabeledSlider( uiParent* p, const char* txt,
				  const char* nm, bool witheditfld )
	: uiGroup(p,"Labeled slider")
{
    slider = new uiSlider( this, nm, witheditfld );
    lbl = new uiLabel( this, txt, slider );
    setHAlignObj( slider );
}
