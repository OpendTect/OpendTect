/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidial.cc,v 1.1 2010-01-13 08:12:40 cvsnanne Exp $";

#include "uidial.h"
#include "i_qdial.h"
#include "uiobjbody.h"

#include "ranges.h"



// TODO: Combine with uiSlider

class uiDialBody : public uiObjBodyImpl<uiDial,QDial>
{
public:

                        uiDialBody(uiDial&,uiParent*,const char*);

    virtual		~uiDialBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

private:

    i_DialMessenger&	messenger_;

};


uiDialBody::uiDialBody( uiDial& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiDial,QDial>(handle,p,nm)
    , messenger_( *new i_DialMessenger(this,&handle) )
{
    setHSzPol( uiObject::Medium );
    setFocusPolicy( Qt::WheelFocus );
    setNotchesVisible( true );
}


//------------------------------------------------------------------------------

uiDial::uiDial( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , sliderMoved(this)
    , sliderPressed(this)
    , sliderReleased(this)
{
}


uiDial::~uiDial()
{
}


uiDialBody& uiDial::mkbody( uiParent* p, const char* nm )
{
    body_= new uiDialBody( *this, p, nm );
    return *body_;
}


void uiDial::setValue( int val  )
{ body_->setValue( val ); }

int uiDial::getValue() const
{ return body_->value(); }

void uiDial::setOrientation( Orientation orient )
{ body_->setOrientation( orient == Vertical ?  Qt::Vertical : Qt::Horizontal );}

uiDial::Orientation uiDial::getOrientation() const
{ return (uiDial::Orientation)( (int)body_->orientation() ); }

void uiDial::setInverted( bool yn )
{ body_->setInvertedAppearance( yn ); }

bool uiDial::isInverted() const
{ return body_->invertedAppearance(); }

void uiDial::setInvertedControls( bool yn )
{ body_->setInvertedControls( yn ); }

bool uiDial::hasInvertedControls() const
{ return body_->invertedControls(); }

void uiDial::setWrapping( bool yn )
{ body_->setWrapping( yn ); }

bool uiDial::hasWrapping() const
{ return body_->wrapping(); }

void uiDial::setMinValue( int minval )
{ body_->setMinimum( minval ); }

void uiDial::setMaxValue( int maxval )
{ body_->setMaximum( maxval ); }

int uiDial::minValue() const
{ return body_->minimum(); }

int uiDial::maxValue() const
{ return body_->maximum(); }

void uiDial::setStep( int step )
{ body_->setSingleStep( step ); }

int uiDial::step() const
{ return body_->singleStep(); }


void uiDial::setInterval( const StepInterval<int>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step );
}


void uiDial::getInterval( StepInterval<int>& intv ) const
{
    intv.start = minValue();
    intv.stop = maxValue();
    intv.step = step();
}
