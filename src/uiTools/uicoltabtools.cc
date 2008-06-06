/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
Date:		Aug 2007
 RCS:           $Id: uicoltabtools.cc,v 1.8 2008-06-06 05:23:00 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uicoltabtools.h"
#include "uicanvas.h"
#include "uirgbarray.h"
#include "uigeom.h"
#include "uiworld2ui.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "coltabindex.h"


uiColorTableCanvas::uiColorTableCanvas( uiParent* p, const ColTab::Sequence& ct,
       					bool withalpha, bool vert )
    : uiRGBArrayCanvas(p,mkRGBArr(withalpha))
    , vertical_(vert)
    , ctseq_(ct)
{
    setPrefHeight( vert ? 160 : 25 );
    setPrefWidth( vert ? 30 : 80 );
    setStretch( 0, 0 );

    drawTool().useBackgroundPattern( withalpha );

    newFillNeeded.notify( mCB(this,uiColorTableCanvas,reFill) );
}


uiColorTableCanvas::~uiColorTableCanvas()
{
    delete rgbarr_;
}


uiRGBArray& uiColorTableCanvas::mkRGBArr( bool withalpha )
{
    rgbarr_ = new uiRGBArray( withalpha );
    return *rgbarr_;
}


void uiColorTableCanvas::reFill( CallBacker* )
{
    if ( ctseq_.name().isEmpty() )
	return;

    const int sz0 = rgbarr_->getSize( !vertical_ );
    const int sz1 = rgbarr_->getSize( vertical_ );
    const ColTab::IndexedLookUpTable indextable( ctseq_, sz0 );
    for ( int idx=0; idx<sz0; idx++ )
    {
	const Color color = indextable.colorForIndex( idx );
	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( vertical_ )
		rgbarr_->set( idy, sz0 - 1 - idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }
}
