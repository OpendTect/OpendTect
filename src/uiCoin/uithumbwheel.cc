/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2002
 RCS:           $Id: uithumbwheel.cc,v 1.2 2002-02-13 10:42:31 arend Exp $
________________________________________________________________________

-*/

#include "uithumbwheel.h"
#include "i_qthumbwhl.h"
#include "uiobjbody.h"

#include <qsize.h> 


//------------------------------------------------------------------------------


class uiThumbWheelBody : public uiObjBodyImpl<uiThumbWheel,SoQtThumbWheel>
{
public:

                        uiThumbWheelBody( uiThumbWheel& handle,
				  uiParent* parnt, const char* nm, bool hor);

    virtual int 	nrTxtLines() const			{ return 1; }

private:

    i_ThumbWheelMessenger& messenger_;

};

uiThumbWheelBody::uiThumbWheelBody( uiThumbWheel& handle,uiParent* parnt, 
				    const char* nm, bool hor )
    : uiObjBodyImpl<uiThumbWheel,SoQtThumbWheel>(handle, parnt, nm)
    , messenger_ ( *new i_ThumbWheelMessenger( this, &handle ))
{
    //setStretch( 0, 0 );
    setSzPol( SzPolicySpec().setHSzP( SzPolicySpec::small ) );
    setOrientation( hor ? SoQtThumbWheel::Horizontal 
			: SoQtThumbWheel::Vertical );
}


//------------------------------------------------------------------------------

uiThumbWheel::uiThumbWheel(  uiParent* parnt, const char* nm, bool hor )
    : uiObject( parnt, nm, mkbody(parnt, nm, hor) )
    , wheelPressed(this)
    , wheelMoved(this)
    , wheelReleased(this)
    , lastmv( 0 )
{
}

uiThumbWheelBody& uiThumbWheel::mkbody( uiParent* parnt, const char* nm, 
					bool hor)
{ 
    body_= new uiThumbWheelBody(*this,parnt,nm,hor);
    return *body_; 
}

const char* uiThumbWheel::text() const
{
    result = body_->value();
    return (const char*)result;
}


int uiThumbWheel::getIntValue() const
{
    return mNINT(body_->value());
}


float uiThumbWheel::getValue() const
{
    return body_->value();
}


void uiThumbWheel::setText( const char* t )
{
    setValue( (float)atof(t) );
}


void uiThumbWheel::setValue( int i )
{
    body_->setValue( i );
}


void uiThumbWheel::setValue( float d )
{
    body_->setValue( d );
}

int uiThumbWheel::minValue() const         { return body_->minValue() ; }
int uiThumbWheel::maxValue() const         { return body_->minValue() ; }
void uiThumbWheel::setMinValue( int m )    { body_->setMinValue(m); }
void uiThumbWheel::setMaxValue( int m )    { body_->setMaxValue(m); }
int uiThumbWheel::step() const             { return body_->lineStep() ; }
void uiThumbWheel::setStep ( int s )       { body_->setSteps(s,0); }

