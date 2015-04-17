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
#include "uiflatviewstdcontrol.h"
#include "uigraphicsview.h"
#include "uimenu.h"
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
#include "visplanedatadisplay.h"
#include "zaxistransform.h"

#include "attribsel.h"
#include "mouseevent.h"
#include "survinfo.h"
#include "visseis2ddisplay.h"
#include "view2ddata.h"
#include "view2ddataman.h"


uiODViewer2DMgr::uiODViewer2DMgr( uiODMain* a )
    : appl_(*a)
    , deftrcspercm_(mUdf(float))
    , defzpercm_(mUdf(float))
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
    tifs3d_->addFactory( new uiODVw2DFaultTreeItemFactory, 4500 );
    tifs3d_->addFactory( new uiODVw2DFaultSSTreeItemFactory, 5500 );
    tifs3d_->addFactory( new uiODVw2DPickSetTreeItemFactory, 6500 );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{
    delete tifs2d_; delete tifs3d_;
    deepErase( viewers2d_ );
}


int uiODViewer2DMgr::displayIn2DViewer( DataPack::ID dpid,
					const Attrib::SelSpec& as, bool dowva )
{
    uiODViewer2D* vwr2d = &addViewer2D( -1 );
    const DataPack::ID vwdpid = vwr2d->createFlatDataPack( dpid, 0 );
    vwr2d->setSelSpec( &as, dowva ); vwr2d->setSelSpec( &as, !dowva );
    vwr2d->setUpView( vwdpid, dowva );
    vwr2d->useStoredDispPars( dowva );
    vwr2d->useStoredDispPars( !dowva );
    attachNotifiers( vwr2d );

    uiFlatViewer& fv = vwr2d->viewwin()->viewer();
    FlatView::DataDispPars& ddp = fv.appearance().ddpars_;
    (!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
    fv.handleChange( FlatView::Viewer::DisplayPars );
    return vwr2d->id_;
}


void uiODViewer2DMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    const DataPack::ID id = visServ().getDisplayedDataPackID( visid, attribid );
    if ( id < 0 ) return;

    uiODViewer2D* vwr2d = find2DViewer( visid, true );
    bool isnewvwr = false;
    if ( !vwr2d )
    {
	isnewvwr = true;
	vwr2d = &addViewer2D( visid );
	mAttachCB( vwr2d->viewWinClosed, uiODViewer2DMgr::viewWinClosedCB );
    }
    else
    {
	vwr2d->setWinTitle();
	visServ().fillDispPars( visid, attribid,
		vwr2d->viewwin()->viewer().appearance().ddpars_, dowva );
    }

    const Attrib::SelSpec* as = visServ().getSelSpec(visid,attribid);
    vwr2d->setSelSpec( as, dowva );
    if ( isnewvwr ) vwr2d->setSelSpec( as, !dowva );

    const int version = visServ().currentVersion( visid, attribid );
    const DataPack::ID dpid = vwr2d->createFlatDataPack( id, version );
    vwr2d->setUpView( dpid, dowva );
    if ( !vwr2d->viewwin() )
	{ pErrMsg( "Viewer2D has no main window !?" ); return; }

    ConstRefMan<ZAxisTransform> zat =
	visServ().getZAxisTransform( visServ().getSceneID(visid) );
    vwr2d->setZAxisTransform( const_cast<ZAxisTransform*>(zat.ptr()) );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pd,visServ().getObject(visid));
    if ( zat && pd && !pd->isVerticalPlane() )
	vwr2d->setTrcKeyZSampling( pd->getTrcKeyZSampling(false,true) );

    uiFlatViewer& fv = vwr2d->viewwin()->viewer();
    if ( isnewvwr )
    {
	attachNotifiers( vwr2d );
	FlatView::DataDispPars& ddp = fv.appearance().ddpars_;
	visServ().fillDispPars( visid, attribid, ddp, dowva );
	visServ().fillDispPars( visid, attribid, ddp, !dowva );
	(!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
	mAttachCB( vwr2d->viewControl()->setHomeZoomPushed,
		   uiODViewer2DMgr::homeZoomChangedCB );
    }

    fv.handleChange( FlatView::Viewer::DisplayPars );
}


void uiODViewer2DMgr::mouseClickCB( CallBacker* cb )
{
    if ( !SI().has3D() ) return;

    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() || !meh->event().rightButton() )
	return;

    uiODViewer2D* curvwr2d = find2DViewer( *meh );
    if ( !curvwr2d ) return;

    const TrcKeyZSampling& tkzs = curvwr2d->getTrcKeyZSampling();
    if ( tkzs.hsamp_.survid_ == Survey::GM().get2DSurvID() )
	return;

    uiFlatViewer& curvwr = curvwr2d->viewwin()->viewer( 0 );
    const uiWorldPoint wp = curvwr.getWorld2Ui().transform(meh->event().pos());
    const Coord3 coord = curvwr.getCoord( wp );
    if ( coord.isUdf() ) return;

    const BinID bid = SI().transform( coord );
    const uiString showinltxt = tr("Show In-line %1...").arg( bid.inl() );
    const uiString showcrltxt = tr("Show Cross-line %1...").arg( bid.crl() );
    const uiString showztxt = tr("Show Z-slice %1...")
			     .arg( coord.z * SI().zDomain().userFactor() );

    const bool isflat = tkzs.isFlat();
    const TrcKeyZSampling::Dir dir = tkzs.defaultDir();
    uiMenu menu( "Menu" );
    if ( !isflat || dir!=TrcKeyZSampling::Inl )
	menu.insertAction( new uiAction(showinltxt), 0 );
    if ( !isflat || dir!=TrcKeyZSampling::Crl )
	menu.insertAction( new uiAction(showcrltxt), 1 );
    if ( !isflat || dir!=TrcKeyZSampling::Z )
	menu.insertAction( new uiAction(showztxt), 2 );

    menu.insertAction( new uiAction("Properties..."), 3 );
    const int menuid = menu.exec();
    if ( menuid>=0 && menuid<3 )
    {
	TrcKeyZSampling newtkzs = SI().sampling(true);
	if ( menuid == 0 )
	    newtkzs.hsamp_.setLineRange( Interval<int>(bid.inl(),bid.inl()) );
	else if ( menuid == 1 )
	    newtkzs.hsamp_.setTrcRange( Interval<int>(bid.crl(),bid.crl()) );
	else if ( menuid == 2 )
	    newtkzs.zsamp_ = Interval<float>( coord.z, coord.z );

	create2DViewer( *curvwr2d, newtkzs );
    }
    else if ( menuid == 3 )
	curvwr2d->viewControl()->doPropertiesDialog( 0 );
}


void uiODViewer2DMgr::create2DViewer( const uiODViewer2D& curvwr2d,
				      const TrcKeyZSampling& newsampling )
{
    uiODViewer2D* vwr2d = &addViewer2D( -1 );
    vwr2d->setSelSpec( &curvwr2d.selSpec(true), true );
    vwr2d->setSelSpec( &curvwr2d.selSpec(false), false );
    vwr2d->setTrcKeyZSampling( newsampling );
    vwr2d->setZAxisTransform( curvwr2d.getZAxisTransform() );

    const uiFlatViewer& curvwr = curvwr2d.viewwin()->viewer( 0 );
    if ( curvwr.isVisible(true) )
	vwr2d->setUpView( vwr2d->createDataPack(true), true );
    else if ( curvwr.isVisible(false) )
	vwr2d->setUpView( vwr2d->createDataPack(false), false );

    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	vwr.appearance().ddpars_ = curvwr.appearance().ddpars_;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }

    attachNotifiers( vwr2d );
}


void uiODViewer2DMgr::attachNotifiers( uiODViewer2D* vwr2d )
{
    mAttachCB( vwr2d->viewWinClosed, uiODViewer2DMgr::viewWinClosedCB );
    mAttachCB( vwr2d->viewControl()->setHomeZoomPushed,
	       uiODViewer2DMgr::homeZoomChangedCB );
    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonPressed,
		   uiODViewer2DMgr::mouseClickCB );
    }
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( int visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d, visServ().getObject(visid));
    if ( s2d )
	vwr->setGeomID(  s2d->getGeomID() );

    vwr->setMouseCursorExchange( &appl_.applMgr().mouseCursorExchange() );
    viewers2d_ += vwr;
    return *vwr;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visid_
				  : viewers2d_[idx]->id_;
	if ( vwrid == id )
	    return viewers2d_[idx];
    }

    return 0;
}


void uiODViewer2DMgr::homeZoomChangedCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewStdControl*,control,cb);
    if ( !control )
	return;
    deftrcspercm_ = control->getPositionsPerCM(true);
    if ( control->isVertical() )
	defzpercm_ = control->getPositionsPerCM( false );
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const MouseEventHandler& meh )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	uiODViewer2D* vwr2d = viewers2d_[idx];
	const int vwridx = vwr2d->viewControl()->getViewerIdx( &meh, true );
	if ( vwridx != -1 )
	    return vwr2d;
    }

    return 0;
}


void uiODViewer2DMgr::viewWinClosedCB( CallBacker* cb )
{
    mDynamicCastGet( uiODViewer2D*, vwr2d, cb );
    if ( vwr2d )
	remove2DViewer( vwr2d->id_, false );
}


void uiODViewer2DMgr::remove2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visid_
				  : viewers2d_[idx]->id_;
	if ( vwrid != id )
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
	    uiODViewer2D* curvwr = find2DViewer( visid, true );
	    if ( curvwr ) curvwr->usePar( *vwrpar );
	}
    }
}

