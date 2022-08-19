/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabtools.h"
#include "uigeom.h"
#include "uigraphicsscene.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"

#include "bufstringset.h"
#include "coltab.h"
#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"


uiColorTableCanvas::uiColorTableCanvas( uiParent* p, const ColTab::Sequence& ct,
					bool withalpha, OD::Orientation ori )
    : uiRGBArrayCanvas(p,mkRGBArr(withalpha))
    , orientation_(ori)
    , ctseq_(ct)
    , flipseq_( false )
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


void uiColorTableCanvas::setFlipped( bool yn )
{
    flipseq_ = yn;
}


void uiColorTableCanvas::setOrientation( OD::Orientation hv )
{
    orientation_ = hv;
    setRGB();
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
    const bool isvertical = orientation_==OD::Vertical;
    const int sz0 = rgbarr_->getSize( !isvertical );
    const int sz1 = rgbarr_->getSize( isvertical );
    const ColTab::IndexedLookUpTable indextable( ctseq_, sz0 );
    for ( int idx=0; idx<sz0; idx++ )
    {
	const int colidx = flipseq_ ? sz0-idx-1 : idx;
	const OD::Color color = indextable.colorForIndex( colidx );
	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( isvertical )
		rgbarr_->set( idy, sz0-1-idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }

    uiPixmap pixmap( sz0, sz1 );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    updatePixmap();
}


bool uiColorTableCanvas::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    MouseEvent me( OD::RightButton, pos.x, pos.y );
    const int refnr = beginCmdRecEvent( "rightButtonPressed" );
    getMouseEventHandler().triggerButtonPressed( me );
    endCmdRecEvent( refnr, "rightButtonPressed" );
    return true;
}
