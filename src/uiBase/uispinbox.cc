/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.cc,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"

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


int uiSpinBox::getIntValue() const
{
    return body_->value();
}


double uiSpinBox::getValue() const
{
    return body_->value();
}


void uiSpinBox::setText( const char* t )
{
    setValue( atoi(t) );
}


void uiSpinBox::setValue( int i )
{
    body_->setValue( i );
}


void uiSpinBox::setValue( double d )
{
    body_->setValue( d );
}

