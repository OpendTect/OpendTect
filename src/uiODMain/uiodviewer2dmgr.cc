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
#include "uiflatviewslicepos.h"
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
#include "geom2dintersections.h"
#include "settings.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "visseis2ddisplay.h"
#include "view2ddata.h"
#include "view2ddataman.h"

static const char* sKeyVW2DTrcsPerCM()	{ return "Viewer2D.TrcsPerCM"; }
static const char* sKeyVW2DZPerCM()	{ return "Viewer2D.ZSamplesPerCM"; }

uiODViewer2DMgr::uiODViewer2DMgr( uiODMain* a )
    : appl_(*a)
    , deftrcspercm_(mUdf(float))
    , defzpercm_(mUdf(float))
    , tifs2d_(new uiTreeFactorySet)
    , tifs3d_(new uiTreeFactorySet)
    , l2dintersections_(0)
{
    Settings::common().get( sKeyVW2DTrcsPerCM(), deftrcspercm_ );
    Settings::common().get( sKeyVW2DZPerCM(), defzpercm_ );
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
    delete l2dintersections_;
    delete tifs2d_; delete tifs3d_;
    deepErase( viewers2d_ );
}


int uiODViewer2DMgr::displayIn2DViewer( DataPack::ID dpid,
					const Attrib::SelSpec& as, bool dowva,
       					Pos::GeomID geomid )
{
    uiODViewer2D* vwr2d = &addViewer2D( -1 );
    const DataPack::ID vwdpid = vwr2d->createFlatDataPack( dpid, 0 );
    vwr2d->setSelSpec( &as, dowva ); vwr2d->setSelSpec( &as, !dowva );
    vwr2d->setUpView( vwdpid, dowva );
    if ( geomid != Survey::GM().cUndefGeomID() )
	vwr2d->setGeomID( geomid );
    vwr2d->useStoredDispPars( dowva );
    vwr2d->useStoredDispPars( !dowva );
    attachNotifiers( vwr2d );

    uiFlatViewer& fv = vwr2d->viewwin()->viewer();
    FlatView::DataDispPars& ddp = fv.appearance().ddpars_;
    (!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
    fv.handleChange( FlatView::Viewer::DisplayPars );
    setAllIntersectionPositions();
    return vwr2d->id_;
}


void uiODViewer2DMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    const DataPack::ID id = visServ().getDisplayedDataPackID( visid, attribid );
    if ( id < 0 ) return;

    ConstRefMan<ZAxisTransform> zat =
	visServ().getZAxisTransform( visServ().getSceneID(visid) );

    uiODViewer2D* vwr2d = find2DViewer( visid, true );
    const bool isnewvwr = !vwr2d;
    if ( !vwr2d )
    {
	vwr2d = &addViewer2D( visid );
	vwr2d->setZAxisTransform( const_cast<ZAxisTransform*>(zat.ptr()) );
    }
    else
	visServ().fillDispPars( visid, attribid,
		vwr2d->viewwin()->viewer().appearance().ddpars_, dowva );
    //<-- So that new display parameters are read before the new data is set.
    //<-- This will avoid time lag between updating data and display parameters.

    const Attrib::SelSpec* as = visServ().getSelSpec(visid,attribid);
    vwr2d->setSelSpec( as, dowva );
    if ( isnewvwr ) vwr2d->setSelSpec( as, !dowva );

    const int version = visServ().currentVersion( visid, attribid );
    const DataPack::ID dpid = vwr2d->createFlatDataPack( id, version );
    vwr2d->setUpView( dpid, dowva );
    vwr2d->setWinTitle();

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pd,visServ().getObject(visid));
    if ( zat && pd && !pd->isVerticalPlane() )
	vwr2d->setTrcKeyZSampling( pd->getTrcKeyZSampling(false,true) );

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    if ( isnewvwr )
    {
	attachNotifiers( vwr2d );
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	visServ().fillDispPars( visid, attribid, ddp, dowva );
	visServ().fillDispPars( visid, attribid, ddp, !dowva );
	(!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
    }

    vwr.handleChange( FlatView::Viewer::DisplayPars );
    setAllIntersectionPositions();
}


void uiODViewer2DMgr::mouseClickCB( CallBacker* cb )
{
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
	    newtkzs.zsamp_ = Interval<float>( mCast(float,coord.z),
					      mCast(float,coord.z) );
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
    setAllIntersectionPositions();
}


void uiODViewer2DMgr::attachNotifiers( uiODViewer2D* vwr2d )
{
    mAttachCB( vwr2d->viewWinClosed, uiODViewer2DMgr::viewWinClosedCB );
    mAttachCB( vwr2d->viewControl()->setHomeZoomPushed,
	       uiODViewer2DMgr::homeZoomChangedCB );
    if ( vwr2d->slicePos() )
	mAttachCB( vwr2d->slicePos()->positionChg,
		   uiODViewer2DMgr::vw2DPosChangedCB );
    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonPressed,
		   uiODViewer2DMgr::mouseClickCB );
    }
}


void uiODViewer2DMgr::reCalc2DIntersetionIfNeeded( Pos::GeomID geomid )
{
    if ( intersection2DReCalNeeded(geomid) )
    {
	delete l2dintersections_;
	l2dintersections_ = new Line2DInterSectionSet;
	BufferStringSet lnms;
	TypeSet<Pos::GeomID> geomids;
	SeisIOObjInfo::getLinesWithData( lnms, geomids );
	BendPointFinder2DGeomSet bpfinder( geomids );
	bpfinder.execute();
	Line2DInterSectionFinder intfinder( bpfinder.bendPoints(),
					    *l2dintersections_ );
	intfinder.execute();
    }
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( int visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d, visServ().getObject(visid));
    if ( s2d )
    {
	vwr->setGeomID(  s2d->getGeomID() );
	reCalc2DIntersetionIfNeeded( s2d->getGeomID() );
    }

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
    Settings::common().set( sKeyVW2DTrcsPerCM(), deftrcspercm_ );
    if ( control->isVertical() )
    {
	defzpercm_ = control->getPositionsPerCM( false );
	Settings::common().set( sKeyVW2DZPerCM(), defzpercm_ );
    }

    mSettWrite()
}


void uiODViewer2DMgr::setVWR2DIntersectionPositions( uiODViewer2D* vwr2d )
{
    TrcKeyZSampling::Dir vwr2ddir = vwr2d->getTrcKeyZSampling().defaultDir();
    TypeSet<FlatView::Annotation::AxisData::AuxPosition>& x1intposs =
	vwr2d->viewwin()->viewer().appearance().annot_.x1_.auxposs_;
    TypeSet<FlatView::Annotation::AxisData::AuxPosition>& x2intposs =
	vwr2d->viewwin()->viewer().appearance().annot_.x2_.auxposs_;
    x1intposs.erase(); x2intposs.erase();
    reCalc2DIntersetionIfNeeded( vwr2d->geomID() );

    if ( vwr2d->geomID()!=Survey::GM().cUndefGeomID() ) 
    {
	const int intscidx = intersection2DIdx( vwr2d->geomID() );
	if ( intscidx<0 )
	    return;
	const Line2DInterSection* intsect = (*l2dintersections_)[intscidx];
	for ( int intposidx=0; intposidx<intsect->size(); intposidx++ )
	{
	    const Line2DInterSection::Point& intpos =
		intsect->getPoint( intposidx );
	    FlatView::Annotation::AxisData::AuxPosition newpos;
	    if ( isVWR2DDisplayed(intpos.line) )
		newpos.isbold_ = true;

	    StepInterval<double> x1rg =
		vwr2d->viewwin()->viewer().posRange( true );
	    StepInterval<int> trcrg =
		vwr2d->getTrcKeyZSampling().hsamp_.trcRange();
	    const int posidx = trcrg.getIndex( intpos.mytrcnr );
	    newpos.pos_ = mCast(float,x1rg.atIndex(posidx));
	    newpos.name_ = Survey::GM().getName( intpos.line );
	    x1intposs += newpos;
	}
    }
    else
    {
	for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	{
	    const uiODViewer2D* idxvwr = viewers2d_[vwridx];
	    const TrcKeyZSampling& idxvwrtkzs = idxvwr->getTrcKeyZSampling();
	    TrcKeyZSampling::Dir idxvwrdir = idxvwrtkzs.defaultDir();
	    if ( vwr2d == idxvwr || vwr2ddir==idxvwrdir )
		continue;

	    FlatView::Annotation::AxisData::AuxPosition newpos;
	    newpos.isbold_ = true;

	    if ( vwr2ddir==TrcKeyZSampling::Inl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Crl )
		{
		    newpos.pos_ = (float) idxvwrtkzs.hsamp_.crlRange().start;
		    newpos.name_ = tr( "CRL %1" ).arg( toString(newpos.pos_) );
		    x1intposs += newpos;
		}
		else
		{
		    newpos.pos_ = idxvwrtkzs.zsamp_.start;
		    newpos.name_ = tr( "ZSlice %1" ).arg(toString(newpos.pos_));
		    x2intposs += newpos;
		}
	    }
	    else if ( vwr2ddir==TrcKeyZSampling::Crl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newpos.pos_ = idxvwrtkzs.hsamp_.inlRange().start;
		    newpos.name_ = tr( "INL %1" ).arg( toString(newpos.pos_) );
		    x1intposs += newpos;
		}
		else
		{
		    newpos.pos_ = idxvwrtkzs.zsamp_.start;
		    newpos.name_ = tr( "ZSlice %1" ).arg(toString(newpos.pos_));
		    x2intposs += newpos;
		}
	    }
	    else
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newpos.pos_ = idxvwrtkzs.hsamp_.inlRange().start;
		    newpos.name_ = tr( "INL %1" ).arg( toString(newpos.pos_) );
		    x1intposs += newpos;
		}
		else
		{
		    newpos.pos_ = idxvwrtkzs.hsamp_.crlRange().start;
		    newpos.name_ = tr( "CRL %1" ).arg( toString(newpos.pos_) );
		    x2intposs += newpos;
		}
	    }
	}
    }

    vwr2d->viewwin()->viewer().handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAllIntersectionPositions()
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = viewers2d_[vwridx];
	setVWR2DIntersectionPositions( vwr2d );
    }
}


bool uiODViewer2DMgr::intersection2DReCalNeeded( Pos::GeomID newgeomid ) const
{
    const int intidx = intersection2DIdx( newgeomid );
    return intidx<0;
}


int uiODViewer2DMgr::intersection2DIdx( Pos::GeomID newgeomid ) const
{
    if ( !l2dintersections_ )
	return -1;
    for ( int lidx=0; lidx<l2dintersections_->size(); lidx++ )
    {
	if ( (*l2dintersections_)[lidx]->geomID()==newgeomid )
	    return lidx;
    }

    return -1;

}


bool uiODViewer2DMgr::isVWR2DDisplayed( const Pos::GeomID& geomid ) const
{
    if ( geomid == Survey::GM().cUndefGeomID() )
	return false;

    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->geomID()==geomid )
	    return true;
    }

    return false;
}


void uiODViewer2DMgr::vw2DPosChangedCB( CallBacker* )
{
    setAllIntersectionPositions();
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

    setAllIntersectionPositions();
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

