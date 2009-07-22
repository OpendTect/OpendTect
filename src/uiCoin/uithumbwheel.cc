/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/02/2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uithumbwheel.cc,v 1.11 2009-07-22 16:01:39 cvsbert Exp $";

#include "uithumbwheel.h"
#include "i_qthumbwhl.h"
#include "uiobjbody.h"

#include <qapplication.h>
#include <qevent.h>
#include <qsize.h> 


//------------------------------------------------------------------------------


class uiThumbWheelBody : public uiObjBodyImpl<uiThumbWheel,SoQtThumbWheel>
{
public:

                        uiThumbWheelBody( uiThumbWheel& handle,
				  uiParent* parnt, const char* nm, bool hor);

    //virtual int 	nrTxtLines() const			{ return 1; }

    void		activate();

private:

    i_ThumbWheelMessenger& messenger_;

};

uiThumbWheelBody::uiThumbWheelBody( uiThumbWheel& handle,uiParent* parnt, 
				    const char* nm, bool hor )
    : uiObjBodyImpl<uiThumbWheel,SoQtThumbWheel>(handle, parnt, nm)
    , messenger_ ( *new i_ThumbWheelMessenger( this, &handle ))
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


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User + 0);


void uiThumbWheelBody::activate()
{
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( &messenger_, actevent );
}

//------------------------------------------------------------------------------

uiThumbWheel::uiThumbWheel(  uiParent* parnt, const char* nm, bool hor )
    : uiObject( parnt, nm, mkbody(parnt, nm, hor) )
    , wheelPressed(this)
    , wheelMoved(this)
    , wheelReleased(this)
    , activatedone(this)
    , lastmv( 0 )
{
}

uiThumbWheelBody& uiThumbWheel::mkbody( uiParent* parnt, const char* nm, 
					bool hor)
{ 
    body_= new uiThumbWheelBody(*this,parnt,nm,hor);
    return *body_; 
}


void uiThumbWheel::activate( float angle )
{ 
    activateangle_ = angle;
    body_->activate(); 
}


bool uiThumbWheel::handleEvent( const QEvent* ev )
{
    if ( ev->type() != sQEventActivate ) return false;

    wheelPressed.trigger();
    lastmv += activateangle_;
    setValue( lastmv );
    wheelMoved.trigger();
    wheelReleased.trigger();

    activatedone.trigger();
    return true;
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
