/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
 RCS:           $Id: uispinbox.cc,v 1.1 2001-02-16 17:02:07 arend Exp $
________________________________________________________________________

-*/

#include "uispinbox.h"

#include "i_qspinbox.h"
#include "i_qobjwrap.h"

#include <qsize.h> 


typedef i_QObjWrapper<QSpinBox> i_QSpinBoxBase;

class i_QSpinBox : public i_QSpinBoxBase
//!< Derived QSpinBox, to override mapTextToValue and mapValueToText
{
public:

                        i_QSpinBox( uiObject& client,
                                    uiObject* parnt=0, const char* name=0 )
                        : i_QSpinBoxBase( client, parnt, name ) {}

protected:
    virtual int		mapTextToValue( bool* ok );
    virtual QString	mapValueToText( int v );
};


int i_QSpinBox::mapTextToValue( bool* ok )
{
    uiSpinBox* ptClient = dynamic_cast<uiSpinBox*> ( uiClient() );
    if( ptClient && ptClient->useMappers() )
    {
        return ptClient->mapTextToValue( ok );
    }
    return i_QSpinBoxBase::mapTextToValue( ok );
}


QString i_QSpinBox::mapValueToText( int v )
{
    uiSpinBox* ptClient = dynamic_cast<uiSpinBox*> ( uiClient() );
    if( ptClient && ptClient->useMappers() )
    {
        return QString( ptClient->mapValueToText( v ) );
    }
    return i_QSpinBoxBase::mapValueToText(v);
}

//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox(  uiObject* parnt, const char* nm )
	: uiWrapObj<i_QSpinBox>(new i_QSpinBox(*this,parnt), parnt, nm)
	, _messenger ( *new i_SpinBoxMessenger( mQtThing(), this ))
	, valueChanged(this)
{
}

const QWidget* 	uiSpinBox::qWidget_() const 	{ return mQtThing(); } 

const char* uiSpinBox::text() const
{
    result = mQtThing()->value();
    return (const char*)result;
}


int uiSpinBox::getIntValue() const
{
    return mQtThing()->value();
}


double uiSpinBox::getValue() const
{
    return mQtThing()->value();
}


void uiSpinBox::setText( const char* t )
{
    setValue( atoi(t) );
}


void uiSpinBox::setValue( int i )
{
    mQtThing()->setValue( i );
}


void uiSpinBox::setValue( double d )
{
    mQtThing()->setValue( d );
}

