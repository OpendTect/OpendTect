/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.cc,v 1.5 2007-05-08 18:37:08 cvskris Exp $
________________________________________________________________________

-*/

#include "flatviewaxesdrawer.h"
#include "flatview.h"
#include "datapackbase.h"
#include "iodraw.h"
#include "iodrawtool.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, ioDrawArea& da )
    : DrawAxis2D(&da)
    , vwr_(vwr)
    , altdim0_(0)
{
}


void FlatView::AxesDrawer::draw( uiRect uir, uiWorldRect wr )
{
    setDrawRectangle( &uir ); setup( wr );

    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;

    drawarea_->drawTool().setPenColor( annot.color_ );

    drawAxes( ad1.showannot_, ad2.showannot_, true, true );
    drawGridLines( ad1.showgridlines_, ad2.showgridlines_ );
}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						 BufferString* txt ) const
{
    if ( !isx || mIsUdf(altdim0_) )
	return ::DrawAxis2D::getAnnotTextAndPos( isx, pos, txt );

    const bool usewva = !vwr_.isVisible( false );
    const FlatPosData& pd( usewva ? vwr_.data().pos(true)
	    			  : vwr_.data().pos(false) );
    IndexInfo idxinfo( pd.indexInfo( true, pos ) );
    pos = pd.position( true, idxinfo.nearest_ );

    if ( txt )
    {
	const FlatDataPack* fdp = vwr_.getPack( usewva );
	if ( !fdp ) fdp = vwr_.getPack( !usewva );
	if ( fdp )
	    *txt = fdp->getAltDim0Value( altdim0_, idxinfo.nearest_ );
	else
	    *txt = pos;
    }

    return pos;
}
