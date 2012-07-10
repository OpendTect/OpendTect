/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/02/2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uithumbwheel.cc,v 1.18 2012-07-10 08:05:34 cvskris Exp $";

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
			~uiThumbWheelBody() { delete &messenger_; }

    //virtual int 	nrTxtLines() const			{ return 1; }

private:

    i_ThumbWheelMessenger& messenger_;

};

uiThumbWheelBody::uiThumbWheelBody( uiThumbWheel& hndl,uiParent* parnt, 
				    const char* nm, bool hor )
    : uiObjBodyImpl<uiThumbWheel,SoQtThumbWheel>(hndl, parnt, nm)
    , messenger_ ( *new i_ThumbWheelMessenger( this, &hndl ))
{
    //setStretch( 0, 0 );
    setOrientation( hor ? SoQtThumbWheel::Horizontal 
			: SoQtThumbWheel::Vertical );
    setEnabled( true );

    if( hor ) 
    {
	setPrefWidth(90);
	setPrefHeight(25);
    }
    else
    {
	setPrefHeight(90);
	setPrefWidth(25);
    }

    setRangeBoundaryHandling(SoQtThumbWheel::ACCUMULATE);

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
    return mNINT32(body_->value());
}


float uiThumbWheel::getValue() const
{
    return body_->value();
}


void uiThumbWheel::setText( const char* t )
{
    setValue( toFloat(t) );
}


void uiThumbWheel::setValue( int i )
{
    body_->setValue( i );
}


void uiThumbWheel::setValue( float d )
{
    body_->setValue( d );
}


void uiThumbWheel::move( float angle )
{
    lastmv = getValue() + angle;
    wheelMoved.trigger();
    setValue( lastmv );
}
