/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.cc,v 1.2 2007-03-10 12:13:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatviewaxesdrawer.h"
#include "flatview.h"
#include "iopar.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, ioDrawArea& da )
    : DrawAxis2D(&da)
    , vwr_(vwr)
{
}


void FlatView::AxesDrawer::draw( uiRect uir, uiWorldRect wr )
{
    setDrawRectangle( &uir ); setup( wr );

    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;

    drawAxes( ad1.showannot_, ad2.showannot_, true, true );
    drawGridLines( ad1.showgridlines_, ad2.showgridlines_ );
}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						 BufferString* txt ) const
{
    const BufferString& iopkey = isx ? xioparkey_ : yioparkey_;
    if ( iopkey.isEmpty() )
	return ::DrawAxis2D::getAnnotTextAndPos( isx, pos, txt );

    const bool usewva = !vwr_.isVisible( false );
    const FlatPosData& pd( usewva ? vwr_.data().pos(true)
	    			  : vwr_.data().pos(false) );
    IndexInfo idxinfo( pd.indexInfo( true, pos ) );
    pos = pd.position( true, idxinfo.nearest_ );
    if ( !txt )
	return pos;

    FlatView::Point pt( pos, pos );
    if ( isx )
	pt.y = yrg_.center();
    else
	pt.x = xrg_.center();

    IOPar iop; vwr_.getAuxInfo( pt, iop );
    const char* vstr = iop.find( iopkey );
    if ( vstr && *vstr )
	*txt = vstr;
    else
	*txt = pos;

    return pos;
}
