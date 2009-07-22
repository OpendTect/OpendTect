/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
Date:		Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicoltabtools.cc,v 1.18 2009-07-22 16:01:42 cvsbert Exp $";

#include "uicoltabtools.h"
#include "uirgbarray.h"
#include "uigeom.h"
#include "uigraphicsscene.h"
#include "uiworld2ui.h"

#include "bufstringset.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "coltabindex.h"
#include "pixmap.h"


uiColorTableCanvas::uiColorTableCanvas( uiParent* p, const ColTab::Sequence& ct,
       					bool withalpha, bool vert )
    : uiRGBArrayCanvas(p,mkRGBArr(withalpha))
    , vertical_(vert)
    , ctseq_(ct)
{
    disableImageSave();
    setDragMode( uiGraphicsView::NoDrag );
    scene().useBackgroundPattern( withalpha );
    setRGB();
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


void uiColorTableCanvas::setRGB()
{
    if ( ctseq_.name().isEmpty() )
	return;

    beforeDraw();
    const int sz0 = rgbarr_->getSize( !vertical_ );
    const int sz1 = rgbarr_->getSize( vertical_ );
    const ColTab::IndexedLookUpTable indextable( ctseq_, sz0 );
    for ( int idx=0; idx<sz0; idx++ )
    {
	const Color color = indextable.colorForIndex( idx );
	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( vertical_ )
		rgbarr_->set( idy, sz0-1-idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }

    ioPixmap pixmap( sz0, sz1 );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    draw();
}
