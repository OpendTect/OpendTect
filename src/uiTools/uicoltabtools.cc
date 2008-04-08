/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
Date:		Aug 2007
 RCS:           $Id: uicoltabtools.cc,v 1.4 2008-04-08 05:54:08 cvssatyaki Exp $
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


uiColorTableCanvas::uiColorTableCanvas( uiParent* p, ColTab::Sequence& ct,
       					bool vert )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , vertical_(vert)
    , ctseq_(ct)
    , coltabchgd(this)
{
    newFillNeeded.notify( mCB(this,uiColorTableCanvas,reFill) );
    if ( vertical_ )
    {
	setPrefHeight( 160 ); setPrefWidth( 30 );
	setStretch( 0, 0 );
	const uiBorder brdr( 5, 5, 5, 5 );
	setBorder( brdr );
    }
    else
    {
	setPrefWidth( 70 ); setPrefHeight( 35 );
	setStretch( 0, 0 );
	const uiBorder brdr( 5, 5, 5, 5 );
	setBorder( brdr );
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


void uiColorTableCanvas::setColTab( ColTab::Sequence& colseq )
{
    ctseq_ = colseq;
    reFill(0);
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
		rgbarr_->set( idy, idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }
    forceNewFill();
    coltabchgd.trigger();
}
