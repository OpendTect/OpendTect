/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uislider.h"
#include "i_qslider.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "uiobjbodyimpl.h"
#include "uispinbox.h"

#include "datainpspec.h"
#include "ranges.h"
#include "scaler.h"

#include <QProxyStyle>
#include <QString>
#include <math.h>

mUseQtnamespace

// Own Style to let slider move to clicked mouse position. Based on:
//https://www.qtcentre.org/threads/9208-QSlider-step-customize?p=49035#post49035

class ODSliderStyle : public QProxyStyle
{
public:
using QProxyStyle::QProxyStyle;

int styleHint( QStyle::StyleHint hint, const QStyleOption* option=nullptr,
	       const QWidget* widget=nullptr,
	       QStyleHintReturn* returndata=nullptr ) const override
{
    if ( hint == QStyle::SH_Slider_AbsoluteSetButtons )
	return (Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);

    return QProxyStyle::styleHint( hint, option, widget, returndata );
}

};


//------------------------------------------------------------------------------

class uiSliderBody : public uiObjBodyImpl<uiSliderObj,QSlider>
{
public:

			uiSliderBody(uiSliderObj&,uiParent*,const char*);
    virtual		~uiSliderBody();

    virtual int			wnrTxtLines() const	{ return 1; }

private:

    i_SliderMessenger&	messenger_;

};


uiSliderBody::uiSliderBody( uiSliderObj& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSliderObj,QSlider>(hndl,p,nm)
    , messenger_( *new i_SliderMessenger(this,(uiSlider*)p) )
{
    setHSzPol( uiObject::Medium );
    setFocusPolicy( Qt::WheelFocus );
    setStyle( new ODSliderStyle(this->style()) );
}


uiSliderBody::~uiSliderBody()
{
    delete &messenger_;
}



// uiSliderObj
uiSliderObj::uiSliderObj( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
{
}

uiSliderObj::~uiSliderObj()
{ delete body_; }

uiSliderBody& uiSliderObj::mkbody( uiParent* p, const char* nm )
{
    body_= new uiSliderBody( *this, p, nm );
    return *body_;
}

//------------------------------------------------------------------------------

uiSlider::uiSlider( uiParent* p, const Setup& setup, const char* nm )
    : uiGroup(p,nm)
    , lbl_(0)
    , editfld_(0)
    , logscale_(setup.logscale_)
    , valueChanged(this)
    , sliderMoved(this)
    , sliderPressed(this)
    , sliderReleased(this)
{
    init( setup, nm );
}


uiSlider::~uiSlider()
{
    delete scaler_;
}


void uiSlider::init( const uiSlider::Setup& setup, const char* nm )
{
    slider_ = new uiSliderObj( this, nm );
    const bool isvert = setup.isvertical_;
    slider_->body().setOrientation( isvert ? Qt::Vertical : Qt::Horizontal );
    slider_->body().setStretch( isvert ? 0 : 1, isvert ? 1 : 0 );

    int nrdec = setup.nrdec_;
    if ( nrdec < 0 ) nrdec = 0;
    double factor = pow( 10., -nrdec );
    scaler_ = new LinScaler( 0, factor );

    if ( !setup.lbl_.isEmpty() )
	lbl_ = new uiLabel( this, setup.lbl_ );

    if ( setup.withedit_ )
    {
	valueChanged.notify( mCB(this,uiSlider,sliderMove) );
	editfld_ = new uiLineEdit( this,
			BufferString(setup.lbl_.getFullString()," value") );
	editfld_->setHSzPol( uiObject::Small );
	editfld_->returnPressed.notify( mCB(this,uiSlider,editRetPress) );
	sliderMove(0);
    }

    if ( setup.isvertical_ )
    {
	slider_->setPrefHeight( setup.sldrsize_ );
	slider_->setPrefWidth( 10 );
	if ( lbl_ ) slider_->attach( centeredBelow, lbl_ );
	if ( editfld_ ) editfld_->attach( centeredBelow, slider_ );
    }
    else
    {
	slider_->setPrefWidth( setup.sldrsize_ );
	if ( lbl_ ) slider_->attach( rightOf, lbl_ );
	if ( editfld_ ) editfld_->attach( rightOf, slider_ );
    }

    setInverted( setup.isinverted_ );
    setInvertedControls( setup.isinverted_ );

    setHAlignObj( slider_ );
}


#define mSliderBlockCmdRec	CmdRecStopper cmdrecstopper( slider_ );


void uiSlider::processInput()
{ if ( editfld_ ) setValue( editfld_->getFValue() ); }

void uiSlider::setToolTip( const uiString& tt )
{ slider_->setToolTip( tt ); }


float uiSlider::getLinearFraction() const
{
    float start = slider_->body().minimum();
    float range = slider_->body().maximum() - start;
    return range ? (slider_->body().sliderPosition()-start)/range : 1.0;
}


void uiSlider::setLinearFraction( float frac )
{
    mSliderBlockCmdRec;
    if ( frac>=0.0 && frac<=1.0 )
    {
	const float val = (1-frac)*slider_->body().minimum() +
		frac*slider_->body().maximum();
	slider_->body().setValue( mNINT32(val) );
    }
}


void uiSlider::setScale( float fact, float constant )
{
    const float userval = getFValue();
    scaler_->factor = fact;
    scaler_->constant = constant;
    setValue( userval );
}


int uiSlider::sliderValue( float fval ) const
{
    if ( logscale_ )
    {
	if ( fval <= 0 ) return 0;
	fval = log10( fval );
    }

    return mNINT32( scaler_ ? scaler_->unScale(fval) : fval );
}


float uiSlider::userValue( int ival ) const
{
    double res = scaler_->scale( (double)ival );
    return logscale_ ? pow( 10, res ) : res;
}


void uiSlider::setText( const char* txt )
{ setValue( toFloat(txt) ); }

void uiSlider::setValue( int ival )
{
    slider_->body().setValue( ival );
    if ( editfld_ )
	editfld_->setValue( ival);
}

void uiSlider::setValue( float fval )
{
    mSliderBlockCmdRec;
    int val = sliderValue( fval );
    slider_->body().setValue( val );
    if ( editfld_ )
	editfld_->setValue( fval );
}


const char* uiSlider::text() const
{
    result_ = userValue( slider_->body().value() );
    return (const char*)result_;
}


int uiSlider::getIntValue() const
{ return slider_->body().value(); }


float uiSlider::getFValue() const
{ return userValue( slider_->body().value() ); }

void uiSlider::setTickMarks( TickPosition ticks )
{ slider_->body().setTickPosition( QSlider::TickPosition( (int)ticks ) ); }

uiSlider::TickPosition uiSlider::tickMarks() const
{ return (uiSlider::TickPosition)( (int)slider_->body().tickPosition() ); }


void uiSlider::setOrientation( OD::Orientation orient )
{
    slider_->body().setOrientation(
	orient == OD::Vertical ? Qt::Vertical : Qt::Horizontal );
}


OD::Orientation uiSlider::getOrientation() const
{ return (OD::Orientation)( (int)slider_->body().orientation() ); }

void uiSlider::setInverted( bool yn )
{ slider_->body().setInvertedAppearance( yn ); }

bool uiSlider::isInverted() const
{ return slider_->body().invertedAppearance(); }

void uiSlider::setInvertedControls( bool yn )
{ slider_->body().setInvertedControls( yn ); }

bool uiSlider::hasInvertedControls() const
{ return slider_->body().invertedControls(); }


void uiSlider::setMinValue( float minval )
{
    mSliderBlockCmdRec;
    slider_->body().setMinimum( sliderValue(minval) );
}


void uiSlider::setMaxValue( float maxval )
{
    mSliderBlockCmdRec;
    slider_->body().setMaximum( sliderValue(maxval) );
}


float uiSlider::minValue() const
{
    return userValue( slider_->body().minimum() );
}


float uiSlider::maxValue() const
{
    return userValue( slider_->body().maximum() );
}


void uiSlider::setStep( float stp )
{
    mSliderBlockCmdRec;
    int istep = (int)stp;
    if ( scaler_ )
    {
	const float fstp = stp / scaler_->factor;
	istep = mNINT32( fstp );
    }
    slider_->body().setSingleStep( istep );
    slider_->body().setPageStep( istep );
}


float uiSlider::step() const
{
    return userValue( slider_->body().singleStep() );
}


void uiSlider::setInterval( const StepInterval<int>& intv )
{ setInterval( intv.start, intv.stop, intv.step ); }

void uiSlider::setInterval( int start, int stop, int stp )
{
    slider_->body().setRange( start, stop );
    slider_->body().setSingleStep( stp );
    slider_->body().setPageStep( stp );
}


void uiSlider::setInterval( const StepInterval<float>& intv )
{ setInterval( intv.start, intv.stop, intv.step ); }

void uiSlider::setInterval( float start, float stop, float stp )
{
    setMinValue( start );
    setMaxValue( stop );
    setStep( stp );
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


int uiSlider::tickStep() const
{ return slider_->body().tickInterval() ; }

void uiSlider::setTickStep( int s )
{ slider_->body().setTickInterval(s); }

void uiSlider::setPageStep( int s )
{ slider_->body().setPageStep( s ); }

int uiSlider::pageStep() const
{ return slider_->body().pageStep(); }


void uiSlider::sliderMove( CallBacker* )
{
    const float val = getFValue();
    if ( editfld_ ) editfld_->setValue( val );
}


float uiSlider::editValue() const
{
    return editfld_ ? editfld_->getFValue() : mUdf(float);
}

void uiSlider::editRetPress( CallBacker* )
{
    if ( editfld_ )
	setValue( editfld_->getFValue() );
    else
	valueChanged.trigger();

    if ( valueChanged.cbs_.size() < 2 )
	sliderReleased.trigger();
}


void uiSlider::setNrDecimalsEditFld( int nrdec )
{
    if ( !editfld_ )
	return;

    uiFloatValidator fv;
    fv.nrdecimals_ = nrdec;
    editfld_->setValidator( fv );
}
