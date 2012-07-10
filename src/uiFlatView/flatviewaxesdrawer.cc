/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: flatviewaxesdrawer.cc,v 1.19 2012-07-10 13:27:27 cvsbruno Exp $";

#include "flatviewaxesdrawer.h"
#include "flatview.h"
#include "flatposdata.h"
#include "datapackbase.h"
#include "iodraw.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, uiGraphicsView& view )
    : uiGraphicsSceneAxisMgr(view)
    , vwr_(vwr)
    , altdim0_(mUdf(int))
{
}


void FlatView::AxesDrawer::draw( uiRect uir, uiWorldRect wr )
{
    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;

    //setViewRect(uir);
    //setWorldCoords( wr );

    /* drawAxes( ad1.showannot_, ad2.showannot_, true, true );
    drawGridLines( ad1.showgridlines_, ad2.showgridlines_ );
     */
}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						 BufferString* txt ) const
{
    bool usewva = !vwr_.isVisible( false );
    const FlatDataPack* fdp = vwr_.pack( usewva );
    if ( !fdp )
	{ usewva = !usewva; fdp = vwr_.pack( usewva ); }

    //if ( !isx || mIsUdf(altdim0_) || !fdp )
//	return ::uiGraphicsSceneAxis::getAnnotTextAndPos( isx, pos, txt );

    const FlatPosData& pd = fdp->posData();
    IndexInfo idxinfo( pd.indexInfo( true, pos ) );
    pos = pd.position( true, idxinfo.nearest_ );

    const double altdimval = fdp->getAltDim0Value( altdim0_, idxinfo.nearest_ );
    if ( txt && !mIsUdf(altdimval) )
	*txt = altdimval;
    return pos;
}
