/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.15 2002-11-06 12:58:05 arend Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "datainpspec.h"

#include <qsize.h> 
#include <math.h>


//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,QSlider>
{
public:

                        uiSliderBody( uiSlider& handle,
				      uiParent* parnt, const char* nm);

    virtual		~uiSliderBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

private:

    i_SliderMessenger&	messenger_;

};

uiSliderBody::uiSliderBody( uiSlider& handle,uiParent* parnt, const char* nm )
    : uiObjBodyImpl<uiSlider,QSlider>(handle, parnt, nm)
    , messenger_ ( *new i_SliderMessenger( this, &handle ))
{
    setStretch( 1, 0 );
    setHSzPol( uiObject::medium );
}


//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* parnt, const char* nm, bool witheditfld,
		    int fact_, bool log_ )
    : uiObject( parnt, nm, mkbody(parnt, nm) )
    , valueChanged(this)
    , sliderMoved(this)
    , editfld(0)
    , factor(fact_)
    , logscale(log_)
{
    body_->setOrientation( QSlider::Horizontal );
    setTickMarks( true );
    if ( witheditfld )
    {
	valueChanged.notify( mCB(this,uiSlider,sliderMove) );
	editfld = new uiLineEdit( parnt, FloatInpSpec() );
	editfld->setHSzPol( uiObject::small );
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
//  TODO: use getRealValue()
    result = body_->value();
    return (const char*)result;
}


int uiSlider::getIntValue() const
{
    return (int)getRealValue( body_->value() );
}


double uiSlider::getValue() const
{
    return getRealValue( body_->value() );
}


void uiSlider::setText( const char* t )
{
    char* endptr;
    double res = strtod( t, &endptr );
    int val = getSliderValue( res );
    body_->setValue( val );
}


void uiSlider::setValue( int i )
{
    int val = getSliderValue( (double)i );
    body_->setValue( val );
    if ( editfld ) editfld->setValue( i );
}


void uiSlider::setValue( double d )
{
    int val = getSliderValue( d );
    body_->setValue( val );
    if ( editfld ) editfld->setValue( d );
}

void uiSlider::setTickMarks( bool yn )
{
    body_->setTickmarks ( yn ? QSlider::Above : QSlider::NoMarks );
}

void uiSlider::sliderMove( CallBacker* )
{
    if ( !editfld ) return;
    editfld->setValue( getValue() );
}

void uiSlider::processInput()
{
    if ( editfld ) editValue(0);
}

void uiSlider::editValue( CallBacker* )
{
    if ( !editfld ) return;
    setValue( editfld->getValue() );
}

double uiSlider::minValue() const		
{ 
    return getRealValue( body_->minValue() ); 
}

double uiSlider::maxValue() const
{
    return getRealValue( body_->maxValue() );
}

void uiSlider::setMinValue( double m )	
{
    body_->setMinValue( getSliderValue(m) ); 
}

void uiSlider::setMaxValue( double m )	
{ 
    body_->setMaxValue( getSliderValue(m) ); 
}

int uiSlider::tickStep() const     	{ return body_->tickInterval() ; }
void uiSlider::setTickStep ( int s )	{ body_->setTickInterval(s); }


double uiSlider::getRealValue( int sliderval ) const
{
    double val = (double)sliderval;
    val /= factor;
    return logscale ? pow(10,val) : val;
}

int uiSlider::getSliderValue( double realval ) const
{
    if ( realval <= 0 ) return 0;

    double val;
    val = logscale ? log10(realval) : realval;
    val *= factor;
    return (int)val;
}


uiLabeledSlider::uiLabeledSlider( uiParent* p, const char* txt,
				  const char* nm, bool witheditfld )
	: uiGroup(p,"Labeled slider")
{
    slider = new uiSlider( this, nm, witheditfld );
    lbl = new uiLabel( this, txt, slider );
    setHAlignObj( slider );
}
