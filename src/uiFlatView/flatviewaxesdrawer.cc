/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: flatviewaxesdrawer.cc,v 1.20 2012-07-12 15:04:44 cvsbruno Exp $";

#include "flatviewaxesdrawer.h"
#include "flatview.h"
#include "flatposdata.h"
#include "datapackbase.h"
#include "uigraphicsview.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, uiGraphicsView& view )
    : uiGraphicsSceneAxisMgr(view)
    , vwr_(vwr)
    , altdim0_(mUdf(int))
{}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						    BufferString* txt ) const
{
    bool usewva = !vwr_.isVisible( false );
    const FlatDataPack* fdp = vwr_.pack( usewva );
    if ( !fdp )
	{ usewva = !usewva; fdp = vwr_.pack( usewva ); }

    const FlatPosData& pd = fdp->posData();
    IndexInfo idxinfo( pd.indexInfo( true, pos ) );
    pos = pd.position( true, idxinfo.nearest_ );

    const double altdimval = fdp->getAltDim0Value( altdim0_, idxinfo.nearest_ );
    if ( txt && !mIsUdf(altdimval) )
	*txt = altdimval;
    return pos;
}
