/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.21 2005-11-08 13:30:33 cvsarend Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "datainpspec.h"
#include "ranges.h"

#include <qstring.h> 
#include <math.h>



//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,QSlider>
{
public:

                        uiSliderBody(uiSlider&,uiParent*,const char*);

    virtual		~uiSliderBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

private:

    i_SliderMessenger&	messenger_;

};

#ifdef USEQT4
# define mFocus		Qt
# define mOrientation	Qt
#else
# define mFocus		QWidget
# define mOrientation	QSlider
#endif

uiSliderBody::uiSliderBody( uiSlider& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSlider,QSlider>(handle,p,nm)
    , messenger_( *new i_SliderMessenger(this,&handle) )
{
    setStretch( 1, 0 );
    setHSzPol( uiObject::medium );
    setFocusPolicy( mFocus::WheelFocus );
}

//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* p, const char* nm, int dec, bool log_ )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , sliderMoved(this)
    , logscale(log_)
{
    body_->setOrientation( mOrientation::Vertical );

    if ( dec < 0 ) dec = 0;
    factor = (int)pow(10,(float)dec);
}


uiSliderBody& uiSlider::mkbody( uiParent* p, const char* nm )
{
    body_= new uiSliderBody( *this, p, nm );
    return *body_;
}


int uiSlider::sliderValue( float fval ) const
{
    if ( fval <= 0 ) return 0;

    float res = logscale ? log10(fval) : fval;
    res *= factor;
    return mNINT(res);
}


float uiSlider::userValue( int ival ) const
{
    float res = float(ival) / factor;
    return logscale ? pow(10,res) : res;
}


void uiSlider::setText( const char* txt )
{
    float res = atof( txt );
    int val = sliderValue( res );
    body_->setValue( val );
}


void uiSlider::setValue( float fval  )
{
    int val = sliderValue( fval );
    body_->setValue( val );
}


const char* uiSlider::text() const
{
    result = userValue( body_->value() );
    return (const char*)result;
}


int uiSlider::getIntValue() const
{ return mNINT( userValue(body_->value()) ); }


float uiSlider::getValue() const
{ return userValue( body_->value() ); }


void uiSlider::setTickMarks( TickPosition ticks )
{
    body_->setTickmarks( QSlider::TickPosition( (int)ticks ) );
}


uiSlider::TickPosition uiSlider::tickMarks() const
{
    return (uiSlider::TickPosition)( (int)body_->tickmarks() );
}


void uiSlider::setOrientation( Orientation or_ )
{
    body_->setOrientation( or_ == Vertical ?  
	  mOrientation::Vertical : mOrientation::Horizontal );
}


uiSlider::Orientation uiSlider::getOrientation() const
{
    return (uiSlider::Orientation)( (int)body_->orientation() );
}


void uiSlider::setMinValue( float minval )
{
    body_->setMinValue( sliderValue(minval) );
}


void uiSlider::setMaxValue( float maxval )
{ 
    body_->setMaxValue( sliderValue(maxval) ); 
}


float uiSlider::minValue() const
{
    return userValue( body_->minValue() );
}


float uiSlider::maxValue() const
{
    return userValue( body_->maxValue() );
}


void uiSlider::setStep( float step )
{
    int istep = sliderValue( step );
    body_->setLineStep( istep );
}


float uiSlider::step() const
{
    return userValue( body_->lineStep() );
}


void uiSlider::setInterval( const StepInterval<float>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step );
}


void uiSlider::getInterval( StepInterval<float>& intv ) const
{
    intv.start = minValue();
    intv.stop = maxValue();
    intv.step = step();
}


int uiSlider::tickStep() const     	{ return body_->tickInterval() ; }
void uiSlider::setTickStep( int s )	{ body_->setTickInterval(s); }






uiSliderExtra::uiSliderExtra( uiParent* p, const Setup& s, const char* nm )
    : uiGroup(p,nm)
    , editfld(0)
    , lbl(0)
{
    init( s, nm );
}


uiSliderExtra::uiSliderExtra( uiParent* p, const char* lbl, const char* nm )
    : uiGroup(p,nm)
    , editfld(0)
    , lbl(0)
{
    init( uiSliderExtra::Setup(lbl), nm );
}


void uiSliderExtra::init( const uiSliderExtra::Setup& setup, const char* nm )
{
    slider = new uiSlider( this, nm, setup.nrdec_, setup.logscale_ );
    slider->setOrientation( uiSlider::Horizontal );

    if ( setup.lbl_ != "" )
	lbl = new uiLabel( this, setup.lbl_, slider );

    if ( setup.withedit_ )
    {
	slider->valueChanged.notify( mCB(this,uiSliderExtra,sliderMove) );
	editfld = new uiLineEdit( this, FloatInpSpec() );
	editfld->setHSzPol( uiObject::small );
	editfld->returnPressed.notify( mCB(this,uiSliderExtra,editRetPress) );
	editfld->attach( rightOf, slider );
	sliderMove(0);
    }

    setHAlignObj( slider );
}


void uiSliderExtra::sliderMove( CallBacker* )
{
    float val = slider->getValue();
    if ( editfld ) editfld->setValue( val );
}


void uiSliderExtra::processInput()
{
    if ( editfld )
	slider->setValue( editfld->getValue() );
}


void uiSliderExtra::editRetPress( CallBacker* )
{
    processInput();
}
