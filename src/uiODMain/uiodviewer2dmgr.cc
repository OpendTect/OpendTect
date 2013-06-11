/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
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
#include "uiodvw2dpicksettreeitem.h"
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uitreeitemmanager.h"
#include "uivispartserv.h"

#include "attribsel.h"
#include "survinfo.h"
#include "visseis2ddisplay.h"
#include "visvw2ddata.h"
#include "visvw2ddataman.h"


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
    tifs3d_->addFactory( new uiODVw2DPickSetTreeItemFactory, 6500 );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{ 
    delete tifs2d_; delete tifs3d_;
    deepErase( viewers2d_ );
}


void uiODViewer2DMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    int dtpackid = visServ().getDataPackID( visid, attribid );
    if ( dtpackid < 0 ) return;

    uiODViewer2D* curvwr = find2DViewer( visid );
    bool isnewvwr = false;
    if ( !curvwr )
    {
	isnewvwr = true;
	curvwr = &addViewer2D( visid );
	curvwr->winClosed.notify(
		mCB(this,uiODViewer2DMgr,viewer2DWinClosedCB) );
    }
    else
    {
	curvwr->setWinTitle();
	visServ().fillDispPars( visid, attribid,
		curvwr->viewwin()->viewer().appearance().ddpars_, dowva );
    }

    const Attrib::SelSpec* as = visServ().getSelSpec(visid,attribid);
    curvwr->setSelSpec( as, dowva );

    FixedString dpname = DPM(DataPackMgr::FlatID()).nameOf(dtpackid);
    if ( as && dpname != as->userRef() )
    {
	for ( int idx=0; idx<DPM(DataPackMgr::FlatID()).packs().size(); idx++ )
	{
	    const int tmpdtpackid = 
			DPM(DataPackMgr::FlatID()).packs()[idx]->id();
	    FixedString tmpnm = DPM(DataPackMgr::FlatID()).nameOf(tmpdtpackid);
	    if ( tmpnm == as->userRef() )
	    {
		dtpackid = tmpdtpackid;
		break;
	    }
	}
    }

    curvwr->setUpView( dtpackid, dowva );
    if ( !curvwr->viewwin() )
	{ pErrMsg( "Viewer2D has no main window !?" ); return; }

    FlatView::DataDispPars& ddp =
	curvwr->viewwin()->viewer().appearance().ddpars_;
    if ( isnewvwr ) 
    {
    	visServ().fillDispPars( visid, attribid, ddp, dowva );
    	visServ().fillDispPars( visid, attribid, ddp, !dowva );
    	(!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
    }

    curvwr->viewwin()->viewer().handleChange( FlatView::Viewer::DisplayPars );
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( int visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    		visServ().getObject(visid));
    if ( s2d )
	vwr->setLineSetID(  s2d->lineSetID() );

    vwr->setMouseCursorExchange( &appl_.applMgr().mouseCursorExchange() );
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


void uiODViewer2DMgr::viewer2DWinClosedCB( CallBacker* cb )
{
    mDynamicCastGet( uiODViewer2D*, vwr2d, cb );
    if ( vwr2d )
	remove2DViewer( vwr2d->visid_ );
}


void uiODViewer2DMgr::remove2DViewer( int visid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->visid_ != visid )
	    continue;

	delete viewers2d_.removeSingle( idx );
	return;
    }
}


void uiODViewer2DMgr::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const uiODViewer2D& vwr2d = *viewers2d_[idx];
	if ( !vwr2d.viewwin() ) continue;

	IOPar vwrpar;
	vwrpar.set( sKeyVisID(), viewers2d_[idx]->visid_ );
	bool wva = vwr2d.viewwin()->viewer().appearance().ddpars_.wva_.show_;
	vwrpar.setYN( sKeyWVA(), wva );
	vwrpar.set( sKeyAttrID(), vwr2d.selSpec(wva).id().asInt() ); 
	vwr2d.fillPar( vwrpar );

	iop.mergeComp( vwrpar, toString( idx ) );
    }
}


void uiODViewer2DMgr::usePar( const IOPar& iop ) 
{
    deepErase( viewers2d_ );

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> vwrpar = iop.subselect( toString(idx) );
	if ( !vwrpar || !vwrpar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}
	int visid; bool wva; int attrid; 
	if ( vwrpar->get( sKeyVisID(), visid ) &&
		vwrpar->get( sKeyAttrID(), attrid ) &&
		    vwrpar->getYN( sKeyWVA(), wva ) )
	{
	    const int nrattribs = visServ().getNrAttribs( visid );
	    const int attrnr = nrattribs-1;
	    displayIn2DViewer( visid, attrnr, wva );
	    uiODViewer2D* curvwr = find2DViewer( visid );
	    if ( curvwr ) curvwr->usePar( *vwrpar );
	}
    }
}

