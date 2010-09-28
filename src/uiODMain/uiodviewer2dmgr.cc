/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodviewer2dmgr.cc,v 1.3 2010-09-28 06:02:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodviewer2dmgr.h"

#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiodviewer2d.h"
#include "uiodvw2dfaulttreeitem.h"
#include "uiodvw2dfaultss2dtreeitem.h"
#include "uiodvw2dfaultsstreeitem.h"
#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uitreeitemmanager.h"
#include "uivispartserv.h"

#include "survinfo.h"
#include "visseis2ddisplay.h"


uiODViewer2DMgr::uiODViewer2DMgr( uiODMain* a )
    : appl_(*a)
    , tifs2d_(new uiTreeFactorySet)
    , tifs3d_(new uiTreeFactorySet)
{
    // for relevant 2D datapack
    tifs2d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1000 );
    tifs2d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2000 );
    tifs2d_->addFactory( new uiODVw2DHor2DTreeItemFactory, 3000 );
    tifs2d_->addFactory( new uiODVw2DFaultSS2DTreeItemFactory, 4000 );

    // for relevant 3D datapack
    tifs3d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1500 );
    tifs3d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2500 );
    tifs3d_->addFactory( new uiODVw2DHor3DTreeItemFactory, 3500 );
    tifs3d_->addFactory( new uiODVw2DFaultSSTreeItemFactory, 4500 );
    tifs3d_->addFactory( new uiODVw2DFaultTreeItemFactory, 5500 );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{ 
    delete tifs2d_; delete tifs3d_;
    deepErase( viewers2d_ );
}


void uiODViewer2DMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    uiODViewer2D* curvwr = find2DViewer( visid );

    if ( !curvwr )
	curvwr = &addViewer2D( visid );

    bool hasvwr = curvwr->viewwin_;
    curvwr->setSelSpec( visServ().getSelSpec(visid,attribid), dowva );
    curvwr->setUpView( visServ().getDataPackID(visid,attribid), dowva );
    visServ().fillDispPars( visid, attribid,
	    	curvwr->viewwin_->viewer().appearance().ddpars_, dowva );

    if ( !hasvwr )
	curvwr->viewwin_->viewer().handleChange( FlatView::Viewer::All );
    else
	curvwr->viewwin_->viewer().handleChange(
		dowva ? FlatView::Viewer::WVAPars : FlatView::Viewer::VDPars );
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( int visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    		visServ().getObject(visid));
    if ( s2d )
	vwr->setLineSetID(  s2d->lineSetID() );
    viewers2d_ += vwr;
    return *vwr;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( int visid )
{
    uiODViewer2D* curvwr = 0;
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->visid_ != visid )
	    continue;

	curvwr = viewers2d_[idx];
	break;
    }

    return curvwr;
}


void uiODViewer2DMgr::remove2DViewer( int visid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->visid_ != visid )
	    continue;

	delete viewers2d_.remove( idx );
	return;
    }
}
