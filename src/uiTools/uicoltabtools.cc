/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
Date:		Aug 2007
 RCS:           $Id: uicoltabtools.cc,v 1.2 2007-08-22 09:09:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicoltabcanvas.h"
#include "colortab.h"

uiColorTableCanvas::uiColorTableCanvas( uiParent* p, const ColorTable& ct,
       					bool vert )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , vertical_(vert)
    , ctab_(ct)
{
    newFillNeeded.notify( mCB(this,uiColorTableCanvas,reFill) );
    if ( vertical_ )
    {
	setPrefHeight( 50 ); setPrefWidth( 160 );
	setBorders( uiSize(100,0), uiSize(0,0) );
	setStretch( 0, 1 );
    }
    else
    {
	setPrefWidth( 70 ); setPrefHeight( 35 );
	setBorders( uiSize(0,5), uiSize(0,5) );
	setStretch( 0, 0 );
    }
}


uiColorTableCanvas::~uiColorTableCanvas()
{
    delete rgbarr_;
}


uiRGBArray& uiColorTableCanvas::mkRGBArr()
{
    rgbarr_ = new uiRGBArray;
    return *rgbarr_;
}


void uiColorTableCanvas::reFill( CallBacker* )
{
    const int nrcol = rgbarr_->getSize( !setup_.vertical_ );
    const int sz = rgbarr_->getSize( vertical_ );

    ColorTable ct( ctab_ );
    ct.calcList( nrcol );
    for ( int idx=0; idx<nrcol; idx++ )
    {
	const Color color = ct.tableColor( idx );
	for ( int idy=0; idy<sz; idy++ )
	{
	    if ( vertical_ )
		rgbarr_->set( idy, idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }
}
