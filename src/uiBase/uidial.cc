/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uidial.h"
#include "i_qdial.h"
#include "uiobjbody.h"
#include "i_layout.h"

#include "ranges.h"
#include "uilabel.h"
#include "uilineedit.h"

mUseQtnamespace

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


uiDialBody::uiDialBody( uiDial& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiDial,QDial>(hndle,p,nm)
    , messenger_( *new i_DialMessenger(this,&hndle) )
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
    , startAtTop_( true )
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
{
    mBlockCmdRec;
    if ( startAtTop_ )
    {
	int N = maxValue() - minValue();
	int newval = ( N/2 + val ) % N;
	body_->setValue( newval ); 
    }
    else
	body_->setValue( val );
}

int uiDial::getValue() const
{ 
    if ( startAtTop_)
    {
	int N = maxValue() - minValue();
	return (body_->value() + N/2) % N;
    }
    else
	return body_->value(); 
}

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
{
    mBlockCmdRec;
    body_->setMinimum( minval );
}

void uiDial::setMaxValue( int maxval )
{
    mBlockCmdRec;
    body_->setMaximum( maxval );
}

int uiDial::minValue() const
{ return body_->minimum(); }

int uiDial::maxValue() const
{ return body_->maximum(); }

void uiDial::setStep( int s )
{
    mBlockCmdRec;
    body_->setSingleStep( s );
}

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


void uiDial::setStartAtTop( bool top )
{
	startAtTop_ = top;
}
	

bool uiDial::hasStartAtTop() const
{
	return startAtTop_;
}


uiDialExtra::uiDialExtra( uiParent* p, const Setup& s, const char* nm )
        : uiGroup(p,nm)
	, editfld_(0)
	, lbl_(0)
{
    init( s, nm );
}


void uiDialExtra::init( const uiDialExtra::Setup& setup, const char* nm )
{
    dial_ = new uiDial( this, nm );
    dial_->setPrefWidth( setup.dialsize_ );
    dial_->setPrefHeight( setup.dialsize_ );

    if ( !setup.lbl_.isEmpty() )
	lbl_ = new uiLabel( this, setup.lbl_ );

    if ( setup.withedit_ )
    {
	dial_->valueChanged.notify( mCB(this,uiDialExtra,sliderMove) );
	editfld_ = new uiLineEdit( this, BufferString(setup.lbl_," value") );
	editfld_->setHSzPol( uiObject::Small );
	editfld_->returnPressed.notify( mCB(this,uiDialExtra,editRetPress) );
	sliderMove(0);
    }

    setHSpacing( 50 );
    setHAlignObj( dial_ );
    
    if ( setup.isvertical_ )
    {
	if ( lbl_ ) dial_->attach( centeredBelow, lbl_ );
	if ( editfld_ ) editfld_->attach( centeredBelow, dial_ );
    }
    else
    {
	// to do: attach lbl_ and editfld_ to the borders of the group
	if ( lbl_ ) 
	{
	    //lbl_->attach( leftBorder, 1 );
	    lbl_->attach( centeredLeftOf, dial_ );
	}
	if ( editfld_ ) 
	{
	    //editfld_->attach( rightBorder, 1 );
	    editfld_->attach( centeredRightOf, dial_ );
	}
    }
}


void uiDialExtra::sliderMove( CallBacker* )
{
    if ( editfld_ )
	editfld_->setValue( dial_->getValue() );
}


void uiDialExtra::processInput()
{
    if ( editfld_ )
	dial_->setValue( editfld_->getIntValue() );
}


float uiDialExtra::editValue() const
{
    return editfld_ ? editfld_->getfValue() : mUdf(float);
}

void uiDialExtra::editRetPress( CallBacker* )
{
    processInput();
}

