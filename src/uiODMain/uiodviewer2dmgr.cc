/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodviewer2dmgr.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsview.h"
#include "uimenu.h"
#include "uiodviewer2d.h"
#include "uiodscenemgr.h"
#include "uiodvw2dfaulttreeitem.h"
#include "uiodvw2dfaultss2dtreeitem.h"
#include "uiodvw2dfaultsstreeitem.h"
#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"
#include "uiodvw2dpicksettreeitem.h"
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uitaskrunner.h"
#include "uitreeitemmanager.h"
#include "uivispartserv.h"
#include "zaxistransform.h"

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "emmanager.h"
#include "emobject.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "ioman.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "posinfo2d.h"
#include "flatposdata.h"
#include "randomlinegeom.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "view2ddata.h"
#include "view2ddataman.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vispicksetdisplay.h"
#include "visrandomtrackdisplay.h"

uiODViewer2DMgr::uiODViewer2DMgr( uiODMain* a )
    : appl_(*a)
    , tifs2d_(new uiTreeFactorySet)
    , tifs3d_(new uiTreeFactorySet)
    , l2dintersections_(0)
    , vw2dObjAdded(this)
    , vw2dObjToBeRemoved(this)
{
    // for relevant 2D datapack
    tifs2d_->addFactory( new uiODView2DWiggleVarAreaTreeItemFactory, 1000 );
    tifs2d_->addFactory( new uiODView2DVariableDensityTreeItemFactory, 2000 );
    tifs2d_->addFactory( new uiODView2DHor2DTreeItemFactory, 3000 );
    tifs2d_->addFactory( new uiODView2DFaultSS2DTreeItemFactory, 4000 );
    tifs2d_->addFactory( new uiODView2DPickSetTreeItemFactory, 5000 );

    // for relevant 3D datapack
    tifs3d_->addFactory( new uiODView2DWiggleVarAreaTreeItemFactory, 1500 );
    tifs3d_->addFactory( new uiODView2DVariableDensityTreeItemFactory, 2500 );
    tifs3d_->addFactory( new uiODView2DHor3DTreeItemFactory, 3500 );
    tifs3d_->addFactory( new uiODView2DFaultTreeItemFactory, 4500 );
    tifs3d_->addFactory( new uiODView2DFaultSSTreeItemFactory, 5500 );
    tifs3d_->addFactory( new uiODView2DPickSetTreeItemFactory, 6500 );

    mAttachCB(IOM().surveyChanged,uiODViewer2DMgr::surveyChangedCB);
    mAttachCB(IOM().applicationClosing,uiODViewer2DMgr::applClosing);

    BufferStringSet lnms;
    SeisIOObjInfo::getLinesWithData( lnms, geom2dids_ );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{
    detachAllNotifiers();
    if ( l2dintersections_ )
	deepErase( *l2dintersections_ );
    deleteAndNullPtr( l2dintersections_ );
    deepErase( viewers2d_ );
    delete tifs2d_; delete tifs3d_;
}


int uiODViewer2DMgr::nr2DViewers() const
{ return viewers2d_.size(); }


void uiODViewer2DMgr::cleanup()
{
    if ( l2dintersections_ )
	deepErase( *l2dintersections_ );
    deleteAndNullPtr( l2dintersections_ );
    deepErase( viewers2d_ );
    geom2dids_.erase();
}


void uiODViewer2DMgr::applClosing( CallBacker* )
{ cleanup(); }


void uiODViewer2DMgr::surveyChangedCB( CallBacker* )
{ cleanup(); }


bool uiODViewer2DMgr::isItemPresent( const uiTreeItem* item ) const
{
    for ( int ivwr=0; ivwr<viewers2d_.size(); ivwr++ )
    {
	if ( viewers2d_[ivwr]->isItemPresent(item) )
	    return true;
    }

    return false;
}


void uiODViewer2DMgr::setupCurInterpItem( uiODViewer2D* vwr2d )
{
    const VisID visselobjid = visServ().getCurInterObjID();
    if ( !visselobjid.isValid() )
	return;

    ConstRefMan<visBase::DataObject> dataobj = visServ().getObject(visselobjid);
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hor3ddisp,dataobj.ptr());
    mDynamicCastGet(const visSurvey::Horizon2DDisplay*,hor2ddisp,dataobj.ptr());
    mDynamicCastGet(const visSurvey::FaultDisplay*,fltdisp,dataobj.ptr());
    mDynamicCastGet(const visSurvey::FaultStickSetDisplay*,fltssdisp,
		    dataobj.ptr());
    mDynamicCastGet(const visSurvey::PickSetDisplay*,pickdisp,dataobj.ptr());
    if ( hor3ddisp )
	vwr2d->setupTrackingHorizon3D( hor3ddisp->getObjectID() );
    else if ( hor2ddisp )
	vwr2d->setupTrackingHorizon2D( hor2ddisp->getObjectID() );
    else if ( fltdisp )
	vwr2d->setupNewTempFault( fltdisp->getEMObjectID() );
    else if ( fltssdisp )
    {
	vwr2d->setupNewTempFaultSS( fltssdisp->getEMObjectID() );
	vwr2d->setupNewTempFaultSS2D( fltssdisp->getEMObjectID() );
    }
    else if ( pickdisp )
	vwr2d->setupNewPickSet( pickdisp->getMultiID() );
}


void uiODViewer2DMgr::setupHorizon3Ds( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedHorizon3Ds( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon3D::typeStr(),
				     vwr2d->getSyncSceneID() );
    vwr2d->addHorizon3Ds( emids );
}


void uiODViewer2DMgr::setupHorizon2Ds( uiODViewer2D* vwr2d )
{
    if ( SI().has2D() )
    {
	TypeSet<EM::ObjectID> emids;
	getLoadedHorizon2Ds( emids );
	appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon2D::typeStr(),
					 vwr2d->getSyncSceneID() );
	vwr2d->addHorizon2Ds( emids );
    }
}


void uiODViewer2DMgr::setupFaults( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedFaults( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Fault3D::typeStr(),
				     vwr2d->getSyncSceneID() );
    vwr2d->addFaults( emids );
}


void uiODViewer2DMgr::setupFaultSSs( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedFaultSSs( emids );
    getLoadedFaultSS2Ds( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(),
				     vwr2d->getSyncSceneID() );
    vwr2d->addFaultSSs( emids );
    vwr2d->addFaultSS2Ds( emids );
}


void uiODViewer2DMgr::setupPickSets( uiODViewer2D* vwr2d )
{
    TypeSet<MultiID> pickmids;
    getLoadedPickSets( pickmids );
    appl_.sceneMgr().getLoadedPickSetIDs( pickmids, false,
					  vwr2d->getSyncSceneID() );
    vwr2d->addPickSets( pickmids );
}


Viewer2DID uiODViewer2DMgr::displayIn2DViewer( DataPackID dpid,
					const Attrib::SelSpec& as,
					const FlatView::DataDispPars::VD& pars,
					bool dowva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( dowva,
								      !dowva );
    return displayIn2DViewer( dpid, as, pars, dest );
}


Viewer2DID uiODViewer2DMgr::displayIn2DViewer( DataPackID dpid,
					const Attrib::SelSpec& as,
					const FlatView::DataDispPars::VD& pars,
					FlatView::Viewer::VwrDest dest )
{
    uiODViewer2D* vwr2d = &addViewer2D( VisID::udf() );
    vwr2d->setSelSpec( &as, FlatView::Viewer::Both );
    vwr2d->makeUpView( vwr2d->createFlatDataPack(dpid,0),
		       FlatView::Viewer::Both );
    vwr2d->setWinTitle( false );

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    const bool wva =	dest == FlatView::Viewer::WVA ||
			dest == FlatView::Viewer::Both;
    const bool vd =	dest == FlatView::Viewer::VD ||
			dest == FlatView::Viewer::Both;
    ddp.vd_ = pars;
    ddp.wva_.show_ = wva;
    ddp.vd_.show_ = vd;
    vwr.handleChange( FlatView::Viewer::DisplayPars );
    attachNotifiersAndSetAuxData( vwr2d );
    return vwr2d->ID();
}


Viewer2DID uiODViewer2DMgr::displayIn2DViewer( Viewer2DPosDataSel& posdatasel,
					bool dowva,
					float initialx1pospercm,
					float initialx2pospercm )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( dowva,
								      !dowva );
    return displayIn2DViewer( posdatasel, dest, initialx1pospercm,
			      initialx2pospercm);
}


Viewer2DID uiODViewer2DMgr::displayIn2DViewer( Viewer2DPosDataSel& posdatasel,
					FlatView::Viewer::VwrDest dest,
					float initialx1pospercm,
					float initialx2pospercm )
{
    DataPackID dpid = DataPack::cNoID();
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( posdatasel.selspec_ );
    const bool isrl =
	posdatasel.rdmlineid_.isValid() || !posdatasel.rdmlinemultiid_.isUdf();
    if ( isrl )
    {
	Geometry::RandomLine* rdmline = 0;
	if ( posdatasel.rdmlineid_.isValid() )
	    rdmline = Geometry::RLM().get( posdatasel.rdmlineid_ );
	else
	    rdmline = Geometry::RLM().get( posdatasel.rdmlinemultiid_ );

	if ( !rdmline )
	    return Viewer2DID::udf();

	posdatasel.tkzs_.zsamp_ = rdmline->zRange();
	dpid = attrserv->createRdmTrcsOutput(
		posdatasel.tkzs_.zsamp_, rdmline->ID() );
    }
    else
	dpid = attrserv->createOutput( posdatasel.tkzs_, DataPack::cNoID() );

    if ( dpid==DataPack::cNoID() )
	return Viewer2DID::udf();

    uiODViewer2D* vwr2d = &addViewer2D( VisID::udf() );
    const Attrib::SelSpec& as = posdatasel.selspec_;
    vwr2d->setSelSpec( &as, FlatView::Viewer::Both );
    const Geometry::RandomLine* rdmline =
	Geometry::RLM().get( posdatasel.rdmlinemultiid_ );
    if ( rdmline )
	vwr2d->setRandomLineID( rdmline->ID() );
    vwr2d->setInitialX1PosPerCM( initialx1pospercm );
    vwr2d->setInitialX2PosPerCM( initialx2pospercm );
    vwr2d->makeUpView( vwr2d->createFlatDataPack(dpid,0),
		       FlatView::Viewer::Both );
    DPM(DataPackMgr::SeisID()).unRef( dpid );
    vwr2d->setWinTitle( false );
    vwr2d->useStoredDispPars( dest );

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    const bool wva =	dest == FlatView::Viewer::WVA ||
			dest == FlatView::Viewer::Both;
    const bool vd =	dest == FlatView::Viewer::VD ||
			dest == FlatView::Viewer::Both;
    ddp.wva_.show_ = wva;
    ddp.vd_.show_ = vd;
    vwr.handleChange( FlatView::Viewer::DisplayPars );
    if ( geom2dids_.size() > 0 )
	vwr2d->viewwin()->viewer().setSeisGeomidsToViewer( geom2dids_ );
    attachNotifiersAndSetAuxData( vwr2d );
    return vwr2d->ID();
}


void uiODViewer2DMgr::displayIn2DViewer( VisID visid, int attribid, bool dowva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( dowva,
								      !dowva );
    displayIn2DViewer( visid, attribid, dest );
}


void uiODViewer2DMgr::displayIn2DViewer( VisID visid, int attribid,
					 FlatView::Viewer::VwrDest dest )
{
    const DataPackID id = visServ().getDisplayedDataPackID( visid, attribid );
    if ( !id.isValid() ) return;

    uiODViewer2D* vwr2d = find2DViewer( visid );
    const bool isnewvwr = !vwr2d;
    if ( !vwr2d )
    {
	vwr2d = &addViewer2D( visid );
	ConstRefMan<ZAxisTransform> zat =
		visServ().getZAxisTransform( visServ().getSceneID(visid) );
	vwr2d->setZAxisTransform( const_cast<ZAxisTransform*>(zat.ptr()) );
	ConstRefMan<visBase::DataObject> dataobj = visServ().getObject( visid );
	mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,dataobj.ptr());
	if ( rtd )
	    vwr2d->setRandomLineID( rtd->getRandomLineID() );
    }
    else
	visServ().fillDispPars( visid, attribid,
		vwr2d->viewwin()->viewer().appearance().ddpars_, dest );
    //<-- So that new display parameters are read before the new data is set.
    //<-- This will avoid time lag between updating data and display parameters.

    const Attrib::SelSpec* as = visServ().getSelSpec(visid,attribid);
    if ( isnewvwr )
	vwr2d->setSelSpec( as, FlatView::Viewer::Both );
    else
	vwr2d->setSelSpec( as, dest );

    const int version = visServ().currentVersion( visid, attribid );
    const DataPackID dpid = vwr2d->createFlatDataPack( id, version );
    if ( isnewvwr )
	vwr2d->makeUpView( dpid, FlatView::Viewer::Both );
    else
	vwr2d->makeUpView( dpid, dest );

    vwr2d->setWinTitle( true );

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    if ( isnewvwr )
    {
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	visServ().fillDispPars( visid, attribid, ddp, dest );
    if ( geom2dids_.size() > 0 )
	vwr2d->viewwin()->viewer().setSeisGeomidsToViewer( geom2dids_ );
	attachNotifiersAndSetAuxData( vwr2d );
    }

    vwr.handleChange( FlatView::Viewer::DisplayPars );
}


#define sEPSPixWidth 5.0f

#define mGetAuxAnnotIdx \
    uiODViewer2D* curvwr2d = find2DViewer( *meh ); \
    if ( !curvwr2d ) return; \
    uiFlatViewer& curvwr = curvwr2d->viewwin()->viewer( 0 ); \
    const uiWorldPoint wp = curvwr.getWorld2Ui().transform(meh->event().pos());\
    const Coord3 coord = curvwr.getCoord( wp );\
    if ( coord.isUdf() ) return;\
    const uiWorldPoint wperpixel = curvwr.getWorld2Ui().worldPerPixel(); \
    const float x1eps  = mCast(float,wperpixel.x) * sEPSPixWidth; \
    const int x1auxposidx = \
	curvwr.appearance().annot_.x1_.auxPosIdx( mCast(float,wp.x), x1eps ); \
    const float x2eps  = mCast(float,wperpixel.y) * sEPSPixWidth; \
    const int x2auxposidx = \
	curvwr.appearance().annot_.x2_.auxPosIdx( mCast(float,wp.y), x2eps );

void uiODViewer2DMgr::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() )
	return;

    mGetAuxAnnotIdx

    if ( !selauxannot_.isselected_ )
    {
	if ( curvwr.appearance().annot_.editable_ ) return;

	if ( x1auxposidx<0 && x2auxposidx<0 && selauxannot_.auxposidx_<0 )
	    return;

	if ( selauxannot_.auxposidx_<0 )
	{
	    curvwr.rgbCanvas().setDragMode( uiGraphicsViewBase::NoDrag );
	    MouseCursor::Shape mc = x1auxposidx>=0 ? MouseCursor::SplitH
						   : MouseCursor::SplitV;
	    if ( curvwr2d->geomID()!=Survey::GeometryManager::cUndefGeomID() )
		mc = MouseCursor::PointingHand;
	    MouseCursorManager::mgr()->setOverride( mc );
	}

	if ( x1auxposidx>=0 )
	    selauxannot_ = SelectedAuxAnnot( x1auxposidx, true, false );
	else if ( x2auxposidx>=0 )
	    selauxannot_ = SelectedAuxAnnot( x2auxposidx, false, false );
	else if ( selauxannot_.auxposidx_>=0 )
	{
	    reSetPrevDragMode( curvwr2d );
	    selauxannot_.auxposidx_ = -1;
	}
    }
    else if ( selauxannot_.isValid() && selauxannot_.isselected_ )
    {
	TypeSet<PlotAnnotation>& xauxannot =
	    selauxannot_.isx1_ ? curvwr.appearance().annot_.x1_.auxannot_
			     : curvwr.appearance().annot_.x2_.auxannot_;
	if ( !xauxannot.validIdx(selauxannot_.auxposidx_) )
	    return;

	PlotAnnotation& selauxannot = xauxannot[selauxannot_.auxposidx_];
	if ( selauxannot.isNormal() )
	    return;

	const StepInterval<double> xrg =
	    curvwr2d->viewwin()->viewer().posRange( selauxannot_.isx1_ );
	const int newposidx = xrg.nearestIndex( selauxannot_.isx1_
						? wp.x : wp.y);
	const float newpos = mCast(float, xrg.atIndex(newposidx) );
	selauxannot.pos_ = newpos;
	TrcKeyZSampling::Dir vwr2ddir =
	    curvwr2d->getTrcKeyZSampling().defaultDir();
	if ( (vwr2ddir==TrcKeyZSampling::Inl && selauxannot_.isx1_) ||
	     (vwr2ddir==TrcKeyZSampling::Z && !selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "CRL %1" ).arg( toString(mNINT32(newpos)) );
	else if ( (vwr2ddir==TrcKeyZSampling::Crl && selauxannot_.isx1_) ||
		  (vwr2ddir==TrcKeyZSampling::Z && selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "INL %1" ).arg( toString(mNINT32(newpos)) );
	else if ( (vwr2ddir==TrcKeyZSampling::Inl && !selauxannot_.isx1_) ||
		  (vwr2ddir==TrcKeyZSampling::Crl && !selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "ZSlice %1" ).arg(
		    toString(newpos*curvwr2d->zDomain().userFactor()) );
    }

    setAuxAnnotLineStyles( curvwr, true );
    setAuxAnnotLineStyles( curvwr, false );
    curvwr.handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAuxAnnotLineStyles( uiFlatViewer& vwr, bool forx1 )
{
    FlatView::Annotation& annot = vwr.appearance().annot_;
    TypeSet<PlotAnnotation>& auxannot = forx1 ? annot.x1_.auxannot_
					      : annot.x2_.auxannot_;
    for ( int idx=0; idx<auxannot.size(); idx++ )
	if ( !auxannot[idx].isNormal() )
	    auxannot[idx].linetype_ = PlotAnnotation::Bold;

    const int selannotidx = selauxannot_.auxposidx_;
    if ( !(forx1^selauxannot_.isx1_) && auxannot.validIdx(selannotidx) )
	if ( !auxannot[selannotidx].isNormal() )
	    auxannot[selannotidx].linetype_ = PlotAnnotation::HighLighted;
}


void uiODViewer2DMgr::reSetPrevDragMode( uiODViewer2D* curvwr2d )
{
    uiGraphicsViewBase::ODDragMode prevdragmode = uiGraphicsViewBase::NoDrag;
    if ( curvwr2d->viewControl()->isRubberBandOn() )
	prevdragmode = uiGraphicsViewBase::RubberBandDrag;
    curvwr2d->viewwin()->viewer(0).rgbCanvas().setDragMode( prevdragmode );
    MouseCursorManager::mgr()->restoreOverride();
}


void uiODViewer2DMgr::handleLeftClick( uiODViewer2D* vwr2d )
{
    if ( !vwr2d ) return;
    uiFlatViewer& vwr = vwr2d->viewwin()->viewer( 0 );
    TypeSet<PlotAnnotation>& auxannot =
	selauxannot_.isx1_ ? vwr.appearance().annot_.x1_.auxannot_
			   : vwr.appearance().annot_.x2_.auxannot_;
    const TrcKeyZSampling& tkzs = vwr2d->getTrcKeyZSampling();
    const int selannotidx = selauxannot_.auxposidx_;
    if ( !tkzs.isFlat() || !auxannot.validIdx(selannotidx) )
	return;

    uiODViewer2D* clickedvwr2d = nullptr;
    if ( tkzs.is2D() )
    {
	if ( auxannot[selannotidx].isNormal() )
	    return;

	Line2DInterSection::Point intpoint2d( Survey::GM().cUndefGeomID(),
					      mUdf(int), mUdf(int) );
	const float auxpos = auxannot[selannotidx].pos_;
	intpoint2d = intersectingLineID( vwr2d, auxpos );
	if ( intpoint2d.line==Survey::GM().cUndefGeomID() )
	   return;
	clickedvwr2d = find2DViewer( intpoint2d.line );
    }
    else
    {
	TrcKeyZSampling oldtkzs, newtkzs;
	ConstRefMan<ZAxisTransform> zat = vwr2d->getZAxisTransform();
	if ( zat )
	{
	    oldtkzs.zsamp_ = newtkzs.zsamp_ = zat->getZInterval( false );
	    oldtkzs.zsamp_.step = newtkzs.zsamp_.step = zat->getGoodZStep();
	}

	if ( tkzs.defaultDir()!=TrcKeyZSampling::Inl && selauxannot_.isx1_ )
	{
	    const int auxpos = mNINT32(selauxannot_.oldauxpos_);
	    const int newauxpos = mNINT32(auxannot[selannotidx].pos_);
	    oldtkzs.hsamp_.setLineRange( Interval<int>(auxpos,auxpos) );
	    newtkzs.hsamp_.setLineRange( Interval<int>(newauxpos,newauxpos) );
	}
	else if ( tkzs.defaultDir()!=TrcKeyZSampling::Z && !selauxannot_.isx1_ )
	{
	    const float auxpos = selauxannot_.oldauxpos_;
	    const float newauxpos = auxannot[selannotidx].pos_;
	    oldtkzs.zsamp_ = Interval<float>( auxpos, auxpos );
	    newtkzs.zsamp_ = Interval<float>( newauxpos, newauxpos );
	}
	else
	{
	    const int auxpos = mNINT32(selauxannot_.oldauxpos_);
	    const int newauxpos = mNINT32(auxannot[selannotidx].pos_);
	    oldtkzs.hsamp_.setTrcRange( Interval<int>(auxpos,auxpos) );
	    newtkzs.hsamp_.setTrcRange( Interval<int>(newauxpos,newauxpos) );
	}

	clickedvwr2d = find2DViewer( oldtkzs );
	if ( clickedvwr2d )
	    clickedvwr2d->setPos( newtkzs );
	setAllIntersectionPositions();
    }

    selauxannot_.isselected_ = false;
    selauxannot_.oldauxpos_ = mUdf(float);
    reSetPrevDragMode( vwr2d );
    if ( clickedvwr2d )
	clickedvwr2d->viewwin()->dockParent()->raise();
}


void uiODViewer2DMgr::mouseClickedCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() ||
	 (!meh->event().rightButton() && !meh->event().leftButton()) )
	return;

    handleLeftClick( find2DViewer(*meh) );
}


void uiODViewer2DMgr::mouseClickCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() || meh->event().ctrlStatus() ||
	 (!meh->event().rightButton() && !meh->event().leftButton()) )
	return;

    uiODViewer2D* curvwr2d = find2DViewer( *meh );
    if ( !curvwr2d ) return;

    uiFlatViewer& curvwr = curvwr2d->viewwin()->viewer( 0 );
    const uiWorldPoint wp = curvwr.getWorld2Ui().transform(meh->event().pos());
    const Coord3 coord = curvwr.getCoord( wp );
    if ( coord.isUdf() ) return;

    const uiWorldPoint wperpixel = curvwr.getWorld2Ui().worldPerPixel();
    const float x1eps  = Math::Abs( sCast(float,wperpixel.x)*sEPSPixWidth );
    const int x1auxposidx =
	curvwr.appearance().annot_.x1_.auxPosIdx( sCast(float,wp.x), x1eps );
    const float x2eps  = Math::Abs( sCast(float,wperpixel.y)*sEPSPixWidth );
    const int x2auxposidx =
	curvwr.appearance().annot_.x2_.auxPosIdx( sCast(float,wp.y), x2eps );


    float samplecrdz = sCast(float,coord.z);
    SI().snapZ( samplecrdz );
    if ( meh->event().leftButton() )
    {
	if ( curvwr.appearance().annot_.editable_ ||
	     curvwr2d->geomID()!=mUdfGeomID || (x1auxposidx<0 && x2auxposidx<0))
	    return;

	const bool isx1auxannot = x1auxposidx>=0;
	const int auxannotidx = isx1auxannot ? x1auxposidx : x2auxposidx;
	const FlatView::Annotation::AxisData& axisdata =
			isx1auxannot ? curvwr.appearance().annot_.x1_
				     : curvwr.appearance().annot_.x2_;
	selauxannot_ = SelectedAuxAnnot( auxannotidx, isx1auxannot, true );
	selauxannot_.oldauxpos_ = axisdata.auxannot_[auxannotidx].pos_;
	return;
    }

    uiMenu menu( uiStrings::sMenu() );
    Line2DInterSection::Point intpoint2d( Survey::GM().cUndefGeomID(),
					  mUdf(int), mUdf(int) );
    const TrcKeyZSampling& tkzs = curvwr2d->getTrcKeyZSampling();
    if ( tkzs.is2D() )
    {
	if ( x1auxposidx>=0 &&
	     curvwr.appearance().annot_.x1_.auxannot_[x1auxposidx].isNormal() )
	{
	    intpoint2d = intersectingLineID( curvwr2d, sCast(float,wp.x) );
	    if ( intpoint2d.line==Survey::GM().cUndefGeomID() )
		return;

	    const uiString show2dtxt = m3Dots(tr("Show Line '%1'")).arg(
					Survey::GM().getName(intpoint2d.line) );
	    menu.insertAction( new uiAction(show2dtxt), 0 );
	}
    }
    else
    {
	const BinID bid = SI().transform( coord );
	const uiString showinltxt
			= m3Dots(tr("Show In-line %1")).arg( bid.inl() );
	const uiString showcrltxt
			= m3Dots(tr("Show Cross-line %1")).arg( bid.crl());
	const uiString showztxt = m3Dots(tr("Show Z-slice %1"))
		.arg( mNINT32(samplecrdz*curvwr2d->zDomain().userFactor()) );

	const bool isflat = tkzs.isFlat();
	const TrcKeyZSampling::Dir dir = tkzs.defaultDir();
	if ( !isflat || dir!=TrcKeyZSampling::Inl )
	    menu.insertAction( new uiAction(showinltxt), 1 );
	if ( !isflat || dir!=TrcKeyZSampling::Crl )
	    menu.insertAction( new uiAction(showcrltxt), 2 );
	if ( !isflat || dir!=TrcKeyZSampling::Z )
	    menu.insertAction( new uiAction(showztxt), 3 );
    }

    menu.insertAction( new uiAction(m3Dots(uiStrings::sProperties())), 4 );

    const int menuid = menu.exec();
    if ( menuid>=0 && menuid<4 )
    {
	const BinID bid = SI().transform( coord );
	uiWorldPoint initialcentre( uiWorldPoint::udf() );
	TrcKeyZSampling newtkzs = SI().sampling(true);
	newtkzs.hsamp_.survid_ = tkzs.hsamp_.survid_;
	ConstRefMan<ZAxisTransform> zat = curvwr2d->getZAxisTransform();
	if ( zat )
	{
	    newtkzs.zsamp_ = zat->getZInterval( false );
	    newtkzs.zsamp_.step = zat->getGoodZStep();
	}

	if ( menuid==0 )
	{
	    const Survey::Geometry* geom =
			Survey::GM().getGeometry( intpoint2d.line );
	    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
	    if ( !geom2d )
		return;

	    const PosInfo::Line2DData& l2ddata = geom2d->data();
	    const StepInterval<int> trcnrrg = l2ddata.trcNrRange();
	    const float trcdist =
		l2ddata.distBetween( trcnrrg.start, intpoint2d.linetrcnr );
	    if ( mIsUdf(trcdist) )
		return;

	    initialcentre = uiWorldPoint( double(trcdist), double(samplecrdz) );
	    newtkzs.hsamp_.init( intpoint2d.line );
	    newtkzs.hsamp_.setGeomID( intpoint2d.line );
	}
	else if ( menuid == 1 )
	{
	    newtkzs.hsamp_.setLineRange( Interval<int>(bid.inl(),bid.inl()) );
	    initialcentre.setXY( double(bid.crl()), double(samplecrdz) );
	}
	else if ( menuid == 2 )
	{
	    newtkzs.hsamp_.setTrcRange( Interval<int>(bid.crl(),bid.crl()) );
	    initialcentre.setXY( double(bid.inl()), double(samplecrdz) );
	}
	else if ( menuid == 3 )
	{
	    newtkzs.zsamp_ = Interval<float>( samplecrdz, samplecrdz );
	    initialcentre.setXY( bid.inl(), bid.crl() );
	}

	create2DViewer( *curvwr2d, newtkzs, initialcentre );
    }
    else if ( menuid == 4 )
	curvwr2d->viewControl()->doPropertiesDialog( 0 );
}


void uiODViewer2DMgr::create2DViewer( const uiODViewer2D& curvwr2d,
				      const TrcKeyZSampling& newsampling,
				      const uiWorldPoint& initialcentre )
{
    uiODViewer2D* vwr2d = &addViewer2D( VisID::udf() );
    vwr2d->setSelSpec( &curvwr2d.selSpec(FlatView::Viewer::WVA),
		       FlatView::Viewer::WVA );
    vwr2d->setSelSpec( &curvwr2d.selSpec(FlatView::Viewer::VD),
		       FlatView::Viewer::VD );
    vwr2d->setZAxisTransform( curvwr2d.getZAxisTransform() );
    uiTaskRunner taskr( const_cast<uiODViewer2D&>(curvwr2d).viewerParent() );
    vwr2d->setTrcKeyZSampling( newsampling, &taskr );

    const uiFlatViewStdControl* control = curvwr2d.viewControl();
    vwr2d->setInitialCentre( initialcentre );
    vwr2d->setInitialX1PosPerCM( control->getCurrentPosPerCM(true) );
    if ( newsampling.defaultDir()!=TrcKeyZSampling::Z && curvwr2d.isVertical() )
	vwr2d->setInitialX2PosPerCM( control->getCurrentPosPerCM(false) );

    const uiFlatViewer& curvwr = curvwr2d.viewwin()->viewer( 0 );
    if ( curvwr.isVisible(true) )
	vwr2d->makeUpView( vwr2d->createDataPack(true), FlatView::Viewer::WVA );
    else if ( curvwr.isVisible(false) )
	vwr2d->makeUpView( vwr2d->createDataPack(false), FlatView::Viewer::VD );

    if ( vwr2d->viewControl() && control )
	vwr2d->viewControl()->setEditMode( control->isEditModeOn() );

    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	vwr.appearance().ddpars_ = curvwr.appearance().ddpars_;
	vwr.appearance().setGeoDefaults( vwr2d->isVertical() );
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
    if ( geom2dids_.size() > 0 )
	vwr2d->viewwin()->viewer().setSeisGeomidsToViewer( geom2dids_ );
    attachNotifiersAndSetAuxData( vwr2d );

    if ( vwr2d->viewControl() && control )
	vwr2d->viewControl()->setEditMode( control->isEditModeOn() );
}


void uiODViewer2DMgr::attachNotifiersAndSetAuxData( uiODViewer2D* vwr2d )
{
    mAttachCB( vwr2d->viewWinClosed, uiODViewer2DMgr::viewWinClosedCB );
    if ( vwr2d->slicePos() )
	mAttachCB( vwr2d->slicePos()->positionChg,
		   uiODViewer2DMgr::vw2DPosChangedCB );
    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonPressed,
		   uiODViewer2DMgr::mouseClickCB );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().movement,
		   uiODViewer2DMgr::mouseMoveCB );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonReleased,
		   uiODViewer2DMgr::mouseClickedCB );
    }

    vwr2d->setUpAux();
    setAllIntersectionPositions();
    setupHorizon3Ds( vwr2d );
    setupHorizon2Ds( vwr2d );
    setupFaults( vwr2d );
    setupFaultSSs( vwr2d );
    setupPickSets( vwr2d );
    setupCurInterpItem( vwr2d );
}


void uiODViewer2DMgr::reCalc2DIntersetionIfNeeded( Pos::GeomID geomid )
{
    if ( intersection2DIdx(geomid) < 0 )
    {
	if ( l2dintersections_ )
	    deepErase( *l2dintersections_ );
	delete l2dintersections_;

	if ( geom2dids_.size() == 0 )
	{
	    BufferStringSet lnms;
	    SeisIOObjInfo::getLinesWithData( lnms, geom2dids_ );
	}
	if ( geom2dids_.size() == 0 )
	    return;

	l2dintersections_ = new Line2DInterSectionSet;
	BendPointFinder2DGeomSet bpfinder( geom2dids_ );
	bpfinder.execute();
	Line2DInterSectionFinder intfinder( bpfinder.bendPoints(),
					    *l2dintersections_ );
	intfinder.execute();
    }
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( VisID visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    mAttachCB( vwr->dataMgr()->dataObjAdded, uiODViewer2DMgr::viewObjAdded );
    mAttachCB( vwr->dataMgr()->dataObjToBeRemoved,
	       uiODViewer2DMgr::viewObjToBeRemoved );
    vwr->setMouseCursorExchange( &appl_.applMgr().mouseCursorExchange() );
    viewers2d_ += vwr;
    return *vwr;
}


uiODViewer2D* uiODViewer2DMgr::getParent2DViewer(
					Vis2DID vwr2dobjid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	ObjectSet<View2D::DataObject> vw2dobjs;
	viewers2d_[idx]->getObjects( vw2dobjs );
	for ( int objidx=0; objidx<vw2dobjs.size(); objidx++ )
	{
	    if ( vw2dobjs[objidx]->id()==vwr2dobjid )
		return viewers2d_[idx];
	}
    }

    return nullptr;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( VisID id )
{
    return find2DViewer( id.asInt(), true );
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( Viewer2DID id )
{
    return find2DViewer( id.asInt(), false );
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visID().asInt()
				  : viewers2d_[idx]->ID().asInt();
	if ( vwrid == id )
	    return viewers2d_[idx];
    }

    return nullptr;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const MouseEventHandler& meh )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->viewwin() &&
		viewers2d_[idx]->viewControl()->getViewerIdx(&meh,true) != -1 )
	    return viewers2d_[idx];
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const Pos::GeomID& geomid )
{
    if ( geomid == Survey::GM().cUndefGeomID() )
	return 0;

    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->geomID() == geomid )
	    return viewers2d_[idx];
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isFlat() )
	return 0;

    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->getTrcKeyZSampling() == tkzs )
	    return viewers2d_[idx];
    }

    return 0;
}


void uiODViewer2DMgr::getVWR2DDataGeomIDs(
	const uiODViewer2D* vwr2d, TypeSet<Pos::GeomID>& commongids ) const
{
    commongids.erase();
    if ( vwr2d->geomID()==Survey::GM().cUndefGeomID() )
	return;

    Attrib::DescSet* ads2d = Attrib::eDSHolder().getDescSet( true, false );
    Attrib::DescSet* ads2dns = Attrib::eDSHolder().getDescSet( true, true );
    const Attrib::Desc* wvadesc =
	ads2d->getDesc( vwr2d->selSpec(true).id() );
    if ( !wvadesc )
	wvadesc = ads2dns->getDesc( vwr2d->selSpec(true).id() );
    const Attrib::Desc* vddesc =
	ads2d->getDesc( vwr2d->selSpec(false).id() );
    if ( !vddesc )
	vddesc = ads2dns->getDesc( vwr2d->selSpec(false).id() );

    if ( !wvadesc && !vddesc )
	return;

    const MultiID wvaid( wvadesc ? wvadesc->getStoredID(true).buf()
				 : vddesc->getStoredID(true).buf() );
    const MultiID vdmid( vddesc ? vddesc->getStoredID(true).buf()
				: wvadesc->getStoredID(true).buf() );
    const SeisIOObjInfo wvasi( wvaid );
    const SeisIOObjInfo vdsi( vdmid );
    BufferStringSet wvalnms, vdlnms;
    wvasi.getLineNames( wvalnms );
    vdsi.getLineNames( vdlnms );

    for ( int lidx=0; lidx<wvalnms.size(); lidx++ )
    {
	const char* wvalnm = wvalnms.get(lidx).buf();
	if ( vdlnms.isPresent(wvalnm) )
	    commongids += Survey::GM().getGeomID( wvalnm );
    }
}


void uiODViewer2DMgr::setVWR2DIntersectionPositions( uiODViewer2D* vwr2d )
{
    const TrcKeyZSampling& tkzs = vwr2d->getTrcKeyZSampling();
    if ( !tkzs.isFlat() || !vwr2d->viewwin() ) return;

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer( 0 );
    TypeSet<PlotAnnotation>& x1auxannot = vwr.appearance().annot_.x1_.auxannot_;
    TypeSet<PlotAnnotation>& x2auxannot = vwr.appearance().annot_.x2_.auxannot_;
    x1auxannot.erase(); x2auxannot.erase();

    if ( vwr2d->geomID()!=Survey::GM().cUndefGeomID() )
    {
	reCalc2DIntersetionIfNeeded( vwr2d->geomID() );
	const int intscidx = intersection2DIdx( vwr2d->geomID() );
	if ( intscidx<0 )
	    return;
	const Line2DInterSection* intsect = (*l2dintersections_)[intscidx];
	if ( !intsect )
	    return;

	TypeSet<Pos::GeomID> datagids;
	getVWR2DDataGeomIDs( vwr2d, datagids );
	const StepInterval<double> x1rg = vwr.posRange( true );
	const FlatPosData* posdata = vwr.getFlatPosData( true );
	if ( !posdata )
	    return;
	const StepInterval<int> trcrg = tkzs.hsamp_.trcRange();
	for ( int intposidx=0; intposidx<intsect->size(); intposidx++ )
	{
	    const Line2DInterSection::Point& intpos =
		intsect->getPoint( intposidx );
	    if ( !datagids.isPresent(intpos.line) )
		continue;

	    PlotAnnotation newannot;
	    const uiODViewer2D* curvwr2d = find2DViewer( intpos.line );
	    if ( curvwr2d &&
		    curvwr2d->getZAxisTransform()==vwr2d->getZAxisTransform() )
		newannot.linetype_ = PlotAnnotation::Bold;

	    const int posidx = trcrg.getIndex( intpos.mytrcnr );
	    newannot.pos_ = sCast(float,posdata->position(true,posidx));
	    newannot.txt_ = mToUiStringTodo( Survey::GM().getName(intpos.line));
	    x1auxannot += newannot;
	}
    }
    else
    {
	const TrcKeyZSampling::Dir dir = tkzs.defaultDir();
	for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	{
	    const uiODViewer2D* curvwr2d = viewers2d_[vwridx];
	    const TrcKeyZSampling& idxvwrtkzs = curvwr2d->getTrcKeyZSampling();
	    TrcKeyZSampling::Dir idxvwrdir = idxvwrtkzs.defaultDir();
	    if ( curvwr2d==vwr2d || idxvwrdir==dir || !idxvwrtkzs.isFlat() ||
		    curvwr2d->getZAxisTransform()!=vwr2d->getZAxisTransform() )
		continue;

	    BinID intersecbid = BinID::udf();
	    PlotAnnotation newannot;
	    newannot.linetype_ = PlotAnnotation::Bold;

	    if ( dir == TrcKeyZSampling::Inl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Crl )
		{
		    newannot.pos_ =
			mCast(float,idxvwrtkzs.hsamp_.crlRange().start);
		    newannot.txt_ =
			tr( "CRL %1" ).arg( idxvwrtkzs.hsamp_.crlRange().start);
		    intersecbid = BinID( tkzs.hsamp_.inlRange().start,
					 mNINT32(newannot.pos_) );
		    x1auxannot += newannot;
		}
		else
		{
		    newannot.pos_ = mCast( float, idxvwrtkzs.zsamp_.start );
		    newannot.txt_ =
			tr( "ZSlice %1" ).arg( idxvwrtkzs.zsamp_.start*
					       SI().showZ2UserFactor() );
		    x2auxannot += newannot;
		}
	    }
	    else if ( dir == TrcKeyZSampling::Crl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newannot.pos_ =
			mCast( float, idxvwrtkzs.hsamp_.inlRange().start );
		    newannot.txt_ =
			tr( "INL %1" ).arg( idxvwrtkzs.hsamp_.inlRange().start);
		    x1auxannot += newannot;
		    intersecbid = BinID( mNINT32(newannot.pos_),
					 tkzs.hsamp_.crlRange().start );
		}
		else
		{
		    newannot.pos_ =
			mCast( float, idxvwrtkzs.zsamp_.start );
		    newannot.txt_ =
			tr( "ZSlice %1" ).arg( idxvwrtkzs.zsamp_.start*
					       SI().showZ2UserFactor() );
		    x2auxannot += newannot;
		}
	    }
	    else
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newannot.pos_ =
			mCast( float, idxvwrtkzs.hsamp_.inlRange().start );
		    newannot.txt_ =
			tr( "INL %1" ).arg( idxvwrtkzs.hsamp_.inlRange().start);
		    x1auxannot += newannot;
		}
		else
		{
		    newannot.pos_ =
			mCast( float, idxvwrtkzs.hsamp_.crlRange().start );
		    newannot.txt_ =
			tr( "CRL %1" ).arg( idxvwrtkzs.hsamp_.crlRange().start);
		    x2auxannot += newannot;
		}
	    }
	}
    }

    vwr.handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAllIntersectionPositions()
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = viewers2d_[vwridx];
	setVWR2DIntersectionPositions( vwr2d );
    }

    TypeSet<EM::ObjectID> hor3dids;
    getLoadedHorizon3Ds( hor3dids );
    for ( int idx=0; idx<hor3dids.size(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(hor3dids[idx]))
	if ( !hor3d )
	    continue;
	EM::EMObjectCallbackData cbdata;
	cbdata.event = EM::EMObjectCallbackData::AttribChange;
	hor3d->change.trigger( cbdata );
    }
}


int uiODViewer2DMgr::intersection2DIdx( Pos::GeomID newgeomid ) const
{
    if ( !l2dintersections_ )
	return -1;
    for ( int lidx=0; lidx<l2dintersections_->size(); lidx++ )
    {
	if ( (*l2dintersections_)[lidx] &&
	     (*l2dintersections_)[lidx]->geomID()==newgeomid )
	    return lidx;
    }

    return -1;
}


Line2DInterSection::Point uiODViewer2DMgr::intersectingLineID(
	const uiODViewer2D* vwr2d, float pos ) const
{
    Line2DInterSection::Point udfintpoint( Survey::GM().cUndefGeomID(),
					   mUdf(int), mUdf(int) );

    const int intsecidx = intersection2DIdx( vwr2d->geomID() );
    if ( intsecidx<0 )
	return udfintpoint;

    const Line2DInterSection* int2d = (*l2dintersections_)[intsecidx];
    if ( !int2d ) return udfintpoint;

    const StepInterval<double> vwrxrg =
	vwr2d->viewwin()->viewer().posRange( true );
    StepInterval<int> vwrtrcrg = vwr2d->getTrcKeyZSampling().hsamp_.trcRange();
    const LinScaler x2trc( vwrxrg.start, vwrtrcrg.start,
			   vwrxrg.stop, vwrtrcrg.stop );
    const double trcnrd = x2trc.scale( pos );
    const int trcnr = mNINT32( trcnrd );
    if ( !vwrtrcrg.includes(trcnr,false) )
	return udfintpoint;

    double mindiff = mUdf(double);
    int minidx = -1;
    TypeSet<Pos::GeomID> datagids;
    getVWR2DDataGeomIDs( vwr2d, datagids );
    for ( int idx=0; idx<int2d->size(); idx++ )
    {
	const Line2DInterSection::Point& intpoint = int2d->getPoint( idx );
	if ( !datagids.isPresent(intpoint.line) )
	    continue;

	const double diff = Math::Abs( (double)(intpoint.mytrcnr-trcnrd) );
	if ( diff > mindiff )
	    continue;

	mindiff = diff;
	minidx = idx;
    }

    return minidx==-1 ? udfintpoint : int2d->getPoint( minidx );
}


void uiODViewer2DMgr::vw2DPosChangedCB( CallBacker* )
{
    setAllIntersectionPositions();
}


void uiODViewer2DMgr::viewWinClosedCB( CallBacker* cb )
{
    mDynamicCastGet( uiODViewer2D*, vwr2d, cb );
    if ( vwr2d )
	remove2DViewer( vwr2d->ID() );
}


void uiODViewer2DMgr::remove2DViewer( Viewer2DID id )
{
    remove2DViewer( id.asInt(), false );
}


void uiODViewer2DMgr::remove2DViewer( VisID visid )
{
    remove2DViewer( visid.asInt(), true );
}


void uiODViewer2DMgr::remove2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visID().asInt()
				  : viewers2d_[idx]->ID().asInt();
	if ( vwrid != id )
	    continue;

	mDetachCB( viewers2d_[idx]->dataMgr()->dataObjAdded,
		   uiODViewer2DMgr::viewObjAdded );
	mDetachCB( viewers2d_[idx]->dataMgr()->dataObjToBeRemoved,
		   uiODViewer2DMgr::viewObjToBeRemoved );
	delete viewers2d_.removeSingle( idx );
	setAllIntersectionPositions();
	return;
    }
}


void uiODViewer2DMgr::viewObjAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(Vis2DID,vw2dobjid,cb);
    vw2dObjAdded.trigger( vw2dobjid );
}


void uiODViewer2DMgr::viewObjToBeRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(Vis2DID,vw2dobjid,cb);
    vw2dObjToBeRemoved.trigger( vw2dobjid );
}


void uiODViewer2DMgr::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const uiODViewer2D& vwr2d = *viewers2d_[idx];
	if ( !vwr2d.viewwin() ) continue;

	IOPar vwrpar;
	vwrpar.set( sKeyVisID(), viewers2d_[idx]->visID() );
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

	VisID visid;
	bool wva;
	int attrid;
	if ( vwrpar->get( sKeyVisID(), visid ) &&
		vwrpar->get( sKeyAttrID(), attrid ) &&
		    vwrpar->getYN( sKeyWVA(), wva ) )
	{
	    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest(
								    wva, !wva );
	    const int nrattribs = visServ().getNrAttribs( visid );
	    const int attrnr = nrattribs-1;
	    displayIn2DViewer( visid, attrnr, dest );
	    uiODViewer2D* curvwr = find2DViewer( visid );
	    if ( curvwr ) curvwr->usePar( *vwrpar );
	}
    }
}


void uiODViewer2DMgr::getVwr2DObjIDs( TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getVwr2DObjIDs( vw2dobjids );
}


void uiODViewer2DMgr::getHor3DVwr2DIDs( EM::ObjectID emid,
					TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getHor3DVwr2DIDs( emid, vw2dobjids );
}


void uiODViewer2DMgr::removeHorizon3D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeHorizon3D( emid );
}


void uiODViewer2DMgr::getLoadedHorizon3Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedHorizon3Ds( emids );
}


void uiODViewer2DMgr::addHorizon3Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addHorizon3Ds( emids );
}


void uiODViewer2DMgr::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    addNewTrackingHorizon3D( emid, VisID::udf() );
}


void uiODViewer2DMgr::addNewTrackingHorizon3D( EM::ObjectID emid,
						SceneID sceneid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTrackingHorizon3D( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon3D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getHor2DVwr2DIDs( EM::ObjectID emid,
					TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getHor2DVwr2DIDs( emid, vw2dobjids );
}


void uiODViewer2DMgr::removeHorizon2D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeHorizon2D( emid );
}


void uiODViewer2DMgr::getLoadedHorizon2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedHorizon2Ds( emids );
}


void uiODViewer2DMgr::addHorizon2Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addHorizon2Ds( emids );
}


void uiODViewer2DMgr::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    addNewTrackingHorizon2D( emid, VisID::udf() );
}


void uiODViewer2DMgr::addNewTrackingHorizon2D( EM::ObjectID emid,
						SceneID sceneid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTrackingHorizon2D( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon2D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getFaultVwr2DIDs( EM::ObjectID emid,
					TypeSet<Vis2DID>& vw2dobjids) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getFaultVwr2DIDs( emid, vw2dobjids );
}


void uiODViewer2DMgr::removeFault( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeFault( emid );
}


void uiODViewer2DMgr::getLoadedFaults( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedFaults( emids );
}


void uiODViewer2DMgr::addFaults( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addFaults( emids );
}


void uiODViewer2DMgr::addNewTempFault( EM::ObjectID emid )
{
    addNewTempFault( emid, VisID::udf() );
}


void uiODViewer2DMgr::addNewTempFault( EM::ObjectID emid, SceneID sceneid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTempFault( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Fault3D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getFaultSSVwr2DIDs( EM::ObjectID emid,
					TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getFaultSSVwr2DIDs( emid, vw2dobjids );
}



void uiODViewer2DMgr::removeFaultSS( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeFaultSS( emid );
}


void uiODViewer2DMgr::getLoadedFaultSSs( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedFaultSSs( emids );
}


void uiODViewer2DMgr::addFaultSSs( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addFaultSSs( emids );
}


void uiODViewer2DMgr::addNewTempFaultSS( EM::ObjectID emid )
{
    addNewTempFaultSS( emid, VisID::udf() );
}


void uiODViewer2DMgr::addNewTempFaultSS( EM::ObjectID emid, SceneID sceneid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTempFaultSS( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(),
				     sceneid );
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getFaultSS2DVwr2DIDs( EM::ObjectID emid,
					TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getFaultSS2DVwr2DIDs( emid, vw2dobjids );
}


void uiODViewer2DMgr::removeFaultSS2D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeFaultSS2D( emid );
}


void uiODViewer2DMgr::getLoadedFaultSS2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedFaultSS2Ds( emids );
}


void uiODViewer2DMgr::addFaultSS2Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addFaultSS2Ds( emids );
}


void uiODViewer2DMgr::addNewTempFaultSS2D( EM::ObjectID emid )
{
    addNewTempFaultSS2D( emid, VisID::udf() );
}


void uiODViewer2DMgr::addNewTempFaultSS2D( EM::ObjectID emid, SceneID sceneid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTempFaultSS2D( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(),
				     sceneid );
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid, sceneid );
}



void uiODViewer2DMgr::getPickSetVwr2DIDs( const MultiID& mid,
					  TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getPickSetVwr2DIDs( mid, vw2dobjids );
}


void uiODViewer2DMgr::removePickSet( const MultiID& mid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removePickSet( mid );
}


void uiODViewer2DMgr::getLoadedPickSets( TypeSet<MultiID>& mids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedPickSets( mids );
}


void uiODViewer2DMgr::addPickSets( const TypeSet<MultiID>& mids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addPickSets( mids );
}
