/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.cc,v 1.14 2004-02-02 15:21:46 nanne Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"
#include "uilabel.h"

#include "i_qspinbox.h"
#include "uiobjbody.h"

#include <qsize.h> 


class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,QSpinBox>
{
public:

                        uiSpinBoxBody(uiSpinBox&,uiParent*, const char* );

    virtual		~uiSpinBoxBody()	{ delete &messenger_; }

    virtual int 	nrTxtLines() const	{ return 1; }

protected:

    virtual int		mapTextToValue( bool* ok );
    virtual QString	mapValueToText( int v );

private:

    i_SpinBoxMessenger& messenger_;

};


uiSpinBoxBody::uiSpinBoxBody(uiSpinBox& handle, uiParent* p, const char* nm)
    : uiObjBodyImpl<uiSpinBox,QSpinBox>( handle, p, nm )
    , messenger_( *new i_SpinBoxMessenger( this, &handle) )	
{
    setHSzPol( uiObject::small );
}


int uiSpinBoxBody::mapTextToValue( bool* ok )
{
    if( handle_.useMappers() )
    {
        return handle_.mapTextToValue( ok );
    }
    return QSpinBox::mapTextToValue( ok );
}


QString uiSpinBoxBody::mapValueToText( int v )
{
    if( handle_.useMappers() )
    {
        return QString( handle_.mapValueToText( v ) );
    }
    return QSpinBox::mapValueToText(v);
}

//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox(  uiParent* parnt, const char* nm )
    : uiObject( parnt,nm,mkbody(parnt,nm) )
    , valueChanged(this)
    , dosnap(false)
{
    valueChanged.notify( mCB(this,uiSpinBox,snapToStep) );
}


uiSpinBox::~uiSpinBox()
{
    valueChanged.remove( mCB(this,uiSpinBox,snapToStep) );
}


void uiSpinBox::snapToStep( CallBacker* )
{
    if ( !dosnap ) return;
    const int curvalue = getIntValue();
    const int diff = curvalue - minValue();
    const int step_ = step();
    if ( diff%step_ )
    {
	float ratio = diff / (float)step_;
	const float newval = minValue() + mNINT(ratio)*step_;
	setValue( newval );
    }
}


uiSpinBoxBody& uiSpinBox::mkbody(uiParent* parnt, const char* nm )
{ 
    body_= new uiSpinBoxBody(*this,parnt, nm);
    return *body_; 
}


const char* uiSpinBox::text() const
{
    result = body_->value();
    return (const char*)result;
}


void uiSpinBox::setInterval( StepInterval<int> intv )
{
    setMinValue(intv.start);
    setMaxValue(intv.stop);
    setStep( intv.step ? intv.step : 1 );
}


StepInterval<int> uiSpinBox::getInterval() const
{
    return StepInterval<int>(minValue(),maxValue(),step());
}


int uiSpinBox::getIntValue() const		{ return body_->value(); }
double uiSpinBox::getValue() const	
    { return static_cast<double>( body_->value() ); }

void uiSpinBox::setText( const char* t )	{ setValue( atoi(t) ); }

void uiSpinBox::setValue( int i )		{ body_->setValue( i ); }
void uiSpinBox::setValue( double d )		{ body_->setValue( mNINT(d) ); }

int uiSpinBox::minValue() const			{ return body_->minValue(); }
int uiSpinBox::maxValue() const			{ return body_->maxValue(); }
void uiSpinBox::setMinValue( int m )		{ body_->setMinValue(m); }
void uiSpinBox::setMaxValue( int m )		{ body_->setMaxValue(m); }

int uiSpinBox::step() const			{ return body_->lineStep(); }

void uiSpinBox::setStep( int s, bool dosnap_ )		
{ 
    body_->setLineStep(s);
    if ( dosnap_ ) snapToStep(0);
}


//------------------------------------------------------------------------------

uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const char* txt,
				    const char* nm )
	: uiGroup(p,"Labeled spinBox")
{
    sb = new uiSpinBox( this, nm );
    lbl = new uiLabel( this, txt, sb );
    setHAlignObj( sb );
}
