/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
 RCS:           $Id: uicolor.cc,v 1.6 2002-02-21 09:24:49 nanne Exp $
________________________________________________________________________

-*/

#include "uicolor.h"
#include "uiparent.h"
#include "uibody.h"
#include "qcolordialog.h"

bool select( Color& col, uiParent* parnt, const char* nm, bool withtransp )
{
  
    bool ok;
    QRgb rgb;

    if ( withtransp )
    {
	rgb = QColorDialog::getRgba( (QRgb) col.rgb(), &ok, 
		      parnt ? parnt->body()->qwidget() : 0, nm );
    }
    else
    {
	QColor newcol = QColorDialog::getColor( QColor((QRgb) col.rgb()) , 
		      parnt ? parnt->body()->qwidget() : 0, nm );

	ok = newcol.isValid();
	rgb = newcol.rgb(); 
    }

    if ( ok )	{ col.setRgb( rgb ); }

    return ok;
}


uiColorInput::uiColorInput( uiParent* p, const Color& c, const char* st,
			    const char* txt )
	: uiPushButton( p, txt )
	, color_(c)
	, seltxt_(st)
{
    setBackgroundColor( color_ );
    activated.notify( mCB(this,uiColorInput,pushed) );
}


void uiColorInput::pushed( CallBacker* )
{
    select( color_, parent(), seltxt_ );
    setBackgroundColor( color_ );
}


void uiColorInput::setColor( Color& col )
{
    color_ = col;
    setBackgroundColor( color_ );
}
