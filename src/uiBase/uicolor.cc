/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
 RCS:           $Id: uicolor.cc,v 1.3 2001-05-16 14:57:18 arend Exp $
________________________________________________________________________

-*/

#include "uicolor.h"
#include "uiobj.h"
#include "qcolordialog.h"

bool select( Color& col, uiParent* parnt, const char* nm )
{
  
    bool ok;
    QRgb rgb;

#ifdef SUPPORT_TRANSPARENCY
    rgb = QColorDialog::getRgba( (QRgb) col.rgb(), &ok, 
				 parnt ? &parnt->qWidget() : 0, nm );
#else
    QColor newcol = QColorDialog::getColor( QColor((QRgb) col.rgb()) , 
    					  parnt ? &parnt->qWidget() : 0, nm );

    ok = newcol.isValid();
    rgb = newcol.rgb(); 
#endif

    if( ok ) 
    { 
	col.setRgb( rgb );
    }
    return ok;
}


uiColorInput::uiColorInput( uiObject* p, const Color& c, const char* st,
			    const char* txt )
	: uiPushButton( p, txt )
	, color_(c)
	, seltxt_(st)
{
    setBackgroundColor( color_ );
    notify( mCB(this,uiColorInput,pushed) );
}


void uiColorInput::pushed( CallBacker* )
{
    select( color_, this, seltxt_ );
    setBackgroundColor( color_ );
}
