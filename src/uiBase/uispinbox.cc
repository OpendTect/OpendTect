/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.cc,v 1.6 2001-10-03 09:03:12 nanne Exp $
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

    virtual bool        isSingleLine() const		{ return true; }

protected:

    virtual int		mapTextToValue( bool* ok );
    virtual QString	mapValueToText( int v );

private:

    i_SpinBoxMessenger& messenger_;

};


uiSpinBoxBody::uiSpinBoxBody(uiSpinBox& handle, uiParent* p, const char* nm)
    : uiObjBodyImpl<uiSpinBox,QSpinBox>( handle, p, nm )
    , messenger_( *new i_SpinBoxMessenger( this, &handle) )	{}


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
    , valueChanged(this)				{}

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


int uiSpinBox::getIntValue() const	{ return body_->value(); }
double uiSpinBox::getValue() const	
    { return static_cast<double>( body_->value() ); }

void uiSpinBox::setText( const char* t )	{ setValue( atoi(t) ); }

void uiSpinBox::setValue( int i )	{ body_->setValue( i ); }
void uiSpinBox::setValue( double d )	{ body_->setValue( mNINT(d) ); }

int uiSpinBox::minValue() const		{ return body_->minValue() ; }
int uiSpinBox::maxValue() const		{ return body_->minValue() ; }
void uiSpinBox::setMinValue( int m )	{ body_->setMinValue(m); }
void uiSpinBox::setMaxValue( int m )	{ body_->setMaxValue(m); }
int uiSpinBox::step() const		{ return body_->lineStep() ; }
void uiSpinBox::setStep ( int s )	{ body_->setLineStep(s); }


uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const char* txt,
				    const char* nm )
	: uiGroup(p,"Labeled spinBox")
{
    sb = new uiSpinBox( this, nm );
    lbl = new uiLabel( this, txt, sb );
    setHAlignObj( sb );
}
