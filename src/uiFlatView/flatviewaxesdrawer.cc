/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.cc,v 1.1 2007-03-09 12:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatviewaxesdrawer.h"
#include "flatview.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, ioDrawArea& da )
    : DrawAxis2D(&da)
    , vwr_(vwr)
    , auxnr_(-1)
{
}


void FlatView::AxesDrawer::draw( uiRect uir, uiWorldRect wr )
{
    setDrawRectangle( &uir ); setup( wr );

    const FlatView::Annotation& annot = vwr_.context().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;

    drawAxes( ad1.showannot_, ad2.showannot_, true, true );
    drawGridLines( ad1.showgridlines_, ad2.showgridlines_ );
}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						 BufferString* txt ) const
{
    return ::DrawAxis2D::getAnnotTextAndPos( isx, pos, txt );
}
