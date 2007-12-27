/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uislider.cc,v 1.29 2007-12-27 11:30:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "datainpspec.h"
#include "ranges.h"

#include <QApplication>
#include <QEvent>
#include <QString> 

#include <math.h>



//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,QSlider>
{
public:

                        uiSliderBody(uiSlider&,uiParent*,const char*);

    virtual		~uiSliderBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

    void		activate(float fraction);
    bool		event(QEvent*);
    
protected:

    float		activatefrac_;

private:

    i_SliderMessenger&	messenger_;

};



uiSliderBody::uiSliderBody( uiSlider& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSlider,QSlider>(handle,p,nm)
    , messenger_( *new i_SliderMessenger(this,&handle) )
{
    setStretch( 1, 0 );
    setHSzPol( uiObject::Medium );
    setFocusPolicy( Qt::WheelFocus );
}


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User+0);

void uiSliderBody::activate( float fraction )
{
    activatefrac_ = fraction;
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( this, actevent );
}


bool uiSliderBody::event( QEvent* ev )
{
    if ( ev->type() != sQEventActivate )
	return QSlider::event( ev );

    if ( activatefrac_>=0.0 && activatefrac_<=1.0 )
    {
	handle_.sliderMoved.trigger();
	setValue( mNINT((1-activatefrac_)*minimum()+activatefrac_*maximum()) );
    }

    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* p, const char* nm, int dec, bool logsc )
    : uiObject(p,nm,mkbody(p,nm))
    , logscale(logsc)
    , valueChanged(this)
    , sliderMoved(this)
    , activatedone(this)
{
    body_->setOrientation( Qt::Horizontal );

    if ( dec < 0 ) dec = 0;
    factor = (int)pow(10,(float)dec);
}


uiSliderBody& uiSlider::mkbody( uiParent* p, const char* nm )
{
    body_= new uiSliderBody( *this, p, nm );
    return *body_;
}


void uiSlider::activate( float fraction )
{ body_->activate( fraction ); }


float uiSlider::getLinearFraction() const
{
    float start = body_->minimum();
    float range = body_->maximum() - start;
    return range ? (body_->sliderPosition()-start)/range : 1.0;
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
	  Qt::Vertical : Qt::Horizontal );
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

    if ( !setup.lbl_.isEmpty() )
	lbl = new uiLabel( this, setup.lbl_, slider );

    if ( setup.withedit_ )
    {
	slider->valueChanged.notify( mCB(this,uiSliderExtra,sliderMove) );
	editfld = new uiLineEdit( this, FloatInpSpec() );
	editfld->setHSzPol( uiObject::Small );
	editfld->returnPressed.notify( mCB(this,uiSliderExtra,editRetPress) );
	if ( setup.orientation_ == uiSlider::Horizontal )
	    editfld->attach( rightOf, slider );
	else
	    editfld->attach( centeredBelow, slider );
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
	slider->setValue( editfld->getfValue() );
}


void uiSliderExtra::editRetPress( CallBacker* )
{
    processInput();
}
