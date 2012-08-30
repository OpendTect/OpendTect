/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uislider.cc,v 1.55 2012-08-30 07:52:52 cvsnageswara Exp $";

#include "uislider.h"
#include "i_qslider.h"
#include "uiobjbody.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "datainpspec.h"
#include "ranges.h"
#include "scaler.h"

#include <QString> 

#include <math.h>



//------------------------------------------------------------------------------



class uiSliderBody : public uiObjBodyImpl<uiSlider,mQtclass(QSlider)>
{
public:

                        uiSliderBody(uiSlider&,uiParent*,const char*);

    virtual		~uiSliderBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

private:

    i_SliderMessenger&	messenger_;

};


uiSliderBody::uiSliderBody( uiSlider& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSlider,mQtclass(QSlider)>(hndl,p,nm)
    , messenger_( *new i_SliderMessenger(this,&hndl) )
{
    setHSzPol( uiObject::Medium );
    setFocusPolicy( mQtclass(Qt)::WheelFocus );
}


//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* p, const char* nm, int dec, bool logsc,
		    bool vert )
    : uiObject(p,nm,mkbody(p,nm))
    , logscale(logsc)
    , valueChanged(this)
    , sliderMoved(this)
    , sliderPressed(this)
    , sliderReleased(this)
{
    body_->setOrientation( vert ? mQtclass(Qt)::Vertical
	    			: mQtclass(Qt)::Horizontal );
    body_->setStretch( vert ? 0 : 1, vert ? 1 : 0 );
    
    if ( dec < 0 ) dec = 0;

    double factor = pow( 10., -dec );
    scaler_ = new LinScaler( 0, factor );
}


uiSlider::~uiSlider()
{
    delete scaler_;
}


uiSliderBody& uiSlider::mkbody( uiParent* p, const char* nm )
{
    body_= new uiSliderBody( *this, p, nm );
    return *body_;
}


float uiSlider::getLinearFraction() const
{
    float start = body_->minimum();
    float range = body_->maximum() - start;
    return range ? (body_->sliderPosition()-start)/range : 1.0;
}


void uiSlider::setLinearFraction( float frac )
{
    mBlockCmdRec;
    if ( frac>=0.0 && frac<=1.0 )
    {
	const float val = (1-frac)*body_->minimum() + frac*body_->maximum();
	body_->setValue( mNINT32(val) );
    }
}


void uiSlider::setScale( float fact, float constant )
{
    const float userval = getValue();
    scaler_->factor = fact;
    scaler_->constant = constant;
    setValue( userval );
}


int uiSlider::sliderValue( float fval ) const
{
    if ( logscale )
    {
	if ( fval <= 0 ) return 0;
	fval = log10( fval );
    }

    return mNINT32( scaler_->unScale(fval) );
}


float uiSlider::userValue( int ival ) const
{
    double res = scaler_->scale( (double)ival );

    return logscale ? pow( 10, res ) : res;
}


void uiSlider::setText( const char* txt )
{
    setValue( toFloat(txt) );
}


void uiSlider::setValue( float fval )
{
    mBlockCmdRec;
    int val = sliderValue( fval );
    body_->setValue( val );
}


const char* uiSlider::text() const
{
    result = userValue( body_->value() );
    return (const char*)result;
}


int uiSlider::getIntValue() const
{ return body_->value(); }


float uiSlider::getValue() const
{ return userValue( body_->value() ); }


void uiSlider::setTickMarks( TickPosition ticks )
{
    body_->setTickPosition( mQtclass(QSlider)::TickPosition( (int)ticks ) );
}


uiSlider::TickPosition uiSlider::tickMarks() const
{
    return (uiSlider::TickPosition)( (int)body_->tickPosition() );
}


void uiSlider::setOrientation( Orientation orient )
{
    body_->setOrientation( orient == Vertical ?  
	  mQtclass(Qt)::Vertical : mQtclass(Qt)::Horizontal );
}


uiSlider::Orientation uiSlider::getOrientation() const
{
    return (uiSlider::Orientation)( (int)body_->orientation() );
}


void uiSlider::setInverted( bool yn )
{ body_->setInvertedAppearance( yn ); }

bool uiSlider::isInverted() const
{ return body_->invertedAppearance(); }

void uiSlider::setInvertedControls( bool yn )
{ body_->setInvertedControls( yn ); }

bool uiSlider::hasInvertedControls() const
{ return body_->invertedControls(); }


void uiSlider::setMinValue( float minval )
{
    mBlockCmdRec;
    body_->setMinimum( sliderValue(minval) );
}


void uiSlider::setMaxValue( float maxval )
{ 
    mBlockCmdRec;
    body_->setMaximum( sliderValue(maxval) ); 
}


float uiSlider::minValue() const
{
    return userValue( body_->minimum() );
}


float uiSlider::maxValue() const
{
    return userValue( body_->maximum() );
}


void uiSlider::setStep( float stp )
{
    mBlockCmdRec;
    int istep = (int)stp;
    if ( scaler_ )
    {
	const float fstp = stp / scaler_->factor;
	istep = mNINT32( fstp );
    }
    body_->setSingleStep( istep );
    body_->setPageStep( istep );
}


float uiSlider::step() const
{
    return userValue( body_->singleStep() );
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


void uiSlider::setLinearScale( double constant, double factor )
{
    if ( scaler_ )
    {
	scaler_->constant = constant;
	scaler_->factor =  factor;
    }
}


int uiSlider::tickStep() const     	{ return body_->tickInterval() ; }
void uiSlider::setTickStep( int s )	{ body_->setTickInterval(s); }



#define mGetNm nm ? nm : (s.lbl_.isEmpty() ? "uiSliderExtra" : s.lbl_.buf() )
uiSliderExtra::uiSliderExtra( uiParent* p, const Setup& s, const char* nm )
    : uiGroup(p,mGetNm)
    , editfld(0)
    , lbl(0)
{
    init( s, mGetNm );
}


void uiSliderExtra::init( const uiSliderExtra::Setup& setup, const char* nm )
{
    slider = new uiSlider( this, nm, setup.nrdec_, setup.logscale_,
	    		   setup.isvertical_ );

    if ( !setup.lbl_.isEmpty() )
	lbl = new uiLabel( this, setup.lbl_ );

    if ( setup.withedit_ )
    {
	slider->valueChanged.notify( mCB(this,uiSliderExtra,sliderMove) );
	editfld = new uiLineEdit( this, BufferString(setup.lbl_," value") );
	editfld->setHSzPol( uiObject::Small );
	editfld->returnPressed.notify( mCB(this,uiSliderExtra,editRetPress) );
	sliderMove(0);
    }

    if ( setup.isvertical_ ) 
    {
	slider->setPrefHeight( setup.sldrsize_ );
	slider->setPrefWidth( 10 );
	if ( lbl ) slider->attach( centeredBelow, lbl );
	if ( editfld ) editfld->attach( centeredBelow, slider );
    }
    else
    {
	slider->setPrefWidth( setup.sldrsize_ );
	if ( lbl ) slider->attach( rightOf, lbl );
	if ( editfld ) editfld->attach( rightOf, slider );
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


float uiSliderExtra::editValue() const
{
    return editfld ? editfld->getfValue() : mUdf(float); 
}

void uiSliderExtra::editRetPress( CallBacker* )
{
    processInput();
}
