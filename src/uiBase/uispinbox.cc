/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.cc,v 1.16 2004-04-14 09:56:28 kristofer Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"
#include "uilabel.h"

#include "i_qspinbox.h"
#include "uiobjbody.h"

#include <qvalidator.h>
#include <math.h>


class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,QSpinBox>
{
public:

                        uiSpinBoxBody(uiSpinBox&,uiParent*, const char* );

    virtual		~uiSpinBoxBody();

    virtual int 	nrTxtLines() const	{ return 1; }
    void		setNrDecimals(int);

protected:

    virtual int		mapTextToValue( bool* ok );
    virtual QString	mapValueToText( int v );

private:

    i_SpinBoxMessenger& messenger_;

    QDoubleValidator*	dval;

};


uiSpinBoxBody::uiSpinBoxBody(uiSpinBox& handle, uiParent* p, const char* nm)
    : uiObjBodyImpl<uiSpinBox,QSpinBox>( handle, p, nm )
    , messenger_( *new i_SpinBoxMessenger( this, &handle) )
    , dval(new QDoubleValidator(this,"Validator"))
{
    setHSzPol( uiObject::small );
    setValidator( dval );
}


uiSpinBoxBody::~uiSpinBoxBody()
{
    delete &messenger_;
    delete dval;
}


int uiSpinBoxBody::mapTextToValue( bool* ok )
{
    return (int)(cleanText().toFloat(ok) * handle_.factor);
}


QString uiSpinBoxBody::mapValueToText( int val )
{
    QString s;
    s.setNum( (float)val / handle_.factor );
    return s;
}


void uiSpinBoxBody::setNrDecimals( int dec )
{
    dval->setDecimals( dec );
}

//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox( uiParent* p, int dec, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , dosnap(false)
    , factor(1)
{
    if ( dec < 0 ) dec = 0;
    factor = (int)pow(10,(float)dec);
    body_->setNrDecimals( dec );

    valueChanged.notify( mCB(this,uiSpinBox,snapToStep) );
}


uiSpinBox::~uiSpinBox()
{
    valueChanged.remove( mCB(this,uiSpinBox,snapToStep) );
}


void uiSpinBox::snapToStep( CallBacker* )
{
    if ( !dosnap ) return;
    const int diff = body_->value() - body_->minValue();
    const int step_ = body_->lineStep() ? body_->lineStep() : 1;
    if ( diff%step_ )
    {
	const float ratio = (float)diff / step_;
	const int newval = body_->minValue() + mNINT(ratio)*step_;
	body_->setValue( newval );
    }
}


uiSpinBoxBody& uiSpinBox::mkbody(uiParent* parnt, const char* nm )
{ 
    body_= new uiSpinBoxBody(*this,parnt, nm);
    return *body_; 
}


void uiSpinBox::setInterval( const StepInterval<int>& intv )
{
    setMinValue(intv.start);
    setMaxValue(intv.stop);
    setStep( intv.step ? intv.step : 1 );
}


void uiSpinBox::setInterval( const StepInterval<float>& intv )
{
    setMinValue(intv.start);
    setMaxValue(intv.stop);
    setStep( intv.step ? intv.step : 1 );
}


StepInterval<int> uiSpinBox::getInterval() const
{
    return StepInterval<int>(minValue(),maxValue(),step());
}


StepInterval<float> uiSpinBox::getFInterval() const
{
    return StepInterval<float>( minFValue(), maxFValue(), fstep() );
}


int uiSpinBox::getValue() const
{ return body_->value() / factor; }

float uiSpinBox::getFValue() const	
{ return (float)body_->value() / factor; }

void uiSpinBox::setValue( int val )
{ body_->setValue( val*factor ); }

void uiSpinBox::setValue( float val )
{ body_->setValue( mNINT(val*factor) ); }


void uiSpinBox::setMinValue( int val )
{ body_->setMinValue( val*factor ); }

void uiSpinBox::setMinValue( float val )
{ body_->setMinValue( mNINT(val*factor) ); }

int uiSpinBox::minValue() const
{ return body_->minValue() / factor; }

float uiSpinBox::minFValue() const
{ return (float)body_->minValue() / factor; }


void uiSpinBox::setMaxValue( int val )
{ body_->setMaxValue( val*factor ); }

void uiSpinBox::setMaxValue( float val )
{ body_->setMaxValue( mNINT(val*factor) ); }

int uiSpinBox::maxValue() const
{ return body_->maxValue() / factor; }

float uiSpinBox::maxFValue() const
{ return (float)body_->maxValue() / factor; }


int uiSpinBox::step() const
{ return body_->lineStep() / factor; }

float uiSpinBox::fstep() const
{ return (float)body_->lineStep() / factor; }

void uiSpinBox::setStep( int step_, bool dosnap_ )		
{ setStep( (float)step_, dosnap_ ); }


void uiSpinBox::setStep( float step_, bool dosnap_ )
{
    if ( !step_ ) step_ = 1;
    body_->setLineStep( mNINT(step_*factor) );
    dosnap = dosnap_;
    snapToStep(0);
}

//------------------------------------------------------------------------------

uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const char* txt, int dec,
				    const char* nm )
	: uiGroup(p,"Labeled spinBox")
{
    sb = new uiSpinBox( this, dec, nm );
    lbl = new uiLabel( this, txt, sb );
    setHAlignObj( sb );
}
