/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
 RCS:           $Id: uicolor.cc,v 1.12 2004-04-20 15:06:06 nanne Exp $
________________________________________________________________________

-*/

#include "uicolor.h"
#include "uibutton.h"
#include "uibody.h"
#include "uilabel.h"
#include "qcolordialog.h"
#include "uiparentbody.h"

bool select( Color& col, uiParent* parnt, const char* nm, bool withtransp )
{
  
    bool ok;
    QRgb rgb;

    if ( withtransp )
    {
	rgb = QColorDialog::getRgba( (QRgb) col.rgb(), &ok, 
		      parnt ? parnt->pbody()->qwidget() : 0, nm );
    }
    else
    {
	QColor newcol = QColorDialog::getColor( QColor((QRgb) col.rgb()) , 
		      parnt ? parnt->pbody()->qwidget() : 0, nm );

	ok = newcol.isValid();
	rgb = newcol.rgb(); 
    }

    if ( ok )	{ col.setRgb( rgb ); }

    return ok;
}


uiColorInput::uiColorInput( uiParent* p, const Color& c, const char* txt,
			    const char* lbltxt, const char* st, bool alpha )
	: uiGroup(p,"Color input")
	, color_(c)
	, seltxt_(st)
    	, collbl(0)
	, colorchanged(this)
	, withalpha(alpha)
{
    uiPushButton* but = new uiPushButton( this, txt );
    but->activated.notify( mCB(this,uiColorInput,selCol) );
    collbl = new uiLabel( this, "   ", but );
    collbl->setBackgroundColor( color_ );
    if ( lbltxt && *lbltxt )
	new uiLabel( this, lbltxt, collbl );

    setHAlignObj( collbl );
}


void uiColorInput::selCol( CallBacker* )
{
    const Color oldcol = color_;
    select( color_, this, seltxt_, withalpha );
    collbl->setBackgroundColor( color_ );
    if ( oldcol != color_ )
	colorchanged.trigger();
}


void uiColorInput::setColor( const Color& col )
{
    color_ = col;
    collbl->setBackgroundColor( color_ );
}
