/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
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
#include "uitreeitem.h"
#include "uivispartserv.h"
#include "zaxistransform.h"

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribprobelayer.h"
#include "coltabseqmgr.h"
#include "emmanager.h"
#include "emobject.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "flatposdata.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "objdisposer.h"
#include "posinfo2d.h"
#include "probeimpl.h"
#include "probemanager.h"
#include "randomlinegeom.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "probe.h"
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
    tifs2d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1000 );
    tifs2d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2000 );
    tifs2d_->addFactory( new uiODVw2DHor2DTreeItemFactory, 3000 );
    tifs2d_->addFactory( new uiODVw2DFaultSS2DTreeItemFactory, 4000 );
    tifs2d_->addFactory( new uiODVw2DPickSetTreeItemFactory, 5000 );

    // for relevant 3D datapack
    tifs3d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1500 );
    tifs3d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2500 );
    tifs3d_->addFactory( new uiODVw2DHor3DTreeItemFactory, 3500 );
    tifs3d_->addFactory( new uiODVw2DFaultTreeItemFactory, 4500 );
    tifs3d_->addFactory( new uiODVw2DFaultSSTreeItemFactory, 5500 );
    tifs3d_->addFactory( new uiODVw2DPickSetTreeItemFactory, 6500 );

    DBM().surveyChanged.notify( mCB(this,uiODViewer2DMgr,surveyChangedCB) );
    BufferStringSet lnms;
    SeisIOObjInfo::getLinesWithData( lnms, geom2dids_ );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{
    detachAllNotifiers();
    if ( l2dintersections_ )
	deepErase( *l2dintersections_ );
    deleteAndZeroPtr( l2dintersections_ );
    deepErase( viewers_ );
    delete tifs2d_; delete tifs3d_;
}


uiODViewer2D* uiODViewer2DMgr::getViewer2D( int idx )
{
    if ( !viewers_.validIdx(idx) )
	return 0;
    mDynamicCastGet(uiODViewer2D*,viewer2d,viewers_[idx]);
    return viewer2d;
}


const uiODViewer2D* uiODViewer2DMgr::getViewer2D( int idx ) const
{ return const_cast<uiODViewer2DMgr*>(this)->getViewer2D(idx); }


int uiODViewer2DMgr::nr2DViewers() const
{ return viewers_.size(); }


void uiODViewer2DMgr::surveyChangedCB( CallBacker* )
{
    if ( l2dintersections_ )
	deepErase( *l2dintersections_ );
    deleteAndZeroPtr( l2dintersections_ );
    deepErase( viewers_ );
    geom2dids_.erase();
}


bool uiODViewer2DMgr::isItemPresent( const uiTreeItem* item ) const
{
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	const uiODViewer2D* viewer2d = getViewer2D( ivwr );
	if ( viewer2d->isItemPresent(item) )
	    return true;
    }

    return false;
}


void uiODViewer2DMgr::setupCurInterpItem( uiODViewer2D* vwr2d )
{
    const int visselobjid = visServ().getCurInterObjID();
    if ( visselobjid<0 )
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
	vwr2d->setupNewPickSet( pickdisp->getDBKey() );
}


void uiODViewer2DMgr::setupHorizon3Ds( uiODViewer2D* vwr2d )
{
    DBKeySet emids;
    getLoadedHorizon3Ds( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon3D::typeStr(), -1 );
    vwr2d->addHorizon3Ds( emids );
}


void uiODViewer2DMgr::setupHorizon2Ds( uiODViewer2D* vwr2d )
{
    if ( SI().has2D() )
    {
	DBKeySet emids;
	getLoadedHorizon2Ds( emids );
	appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon2D::typeStr(), -1 );
	vwr2d->addHorizon2Ds( emids );
    }
}


void uiODViewer2DMgr::setupFaults( uiODViewer2D* vwr2d )
{
    DBKeySet emids;
    getLoadedFaults( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Fault3D::typeStr(), -1 );
    vwr2d->addFaults( emids );
}


void uiODViewer2DMgr::setupFaultSSs( uiODViewer2D* vwr2d )
{
    DBKeySet emids;
    getLoadedFaultSSs( emids );
    getLoadedFaultSS2Ds( emids );
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(), -1 );
    vwr2d->addFaultSSs( emids );
    vwr2d->addFaultSS2Ds( emids );
}


void uiODViewer2DMgr::setupPickSets( uiODViewer2D* vwr2d )
{
    DBKeySet pickmids;
    getLoadedPickSets( pickmids );
    appl_.sceneMgr().getLoadedPickSetIDs( pickmids, false, -1 );
    vwr2d->addPickSets( pickmids );
}


Presentation::ViewerObjID uiODViewer2DMgr::displayIn2DViewer(
	Probe& probe, ProbeLayer::ID curlayid, uiODViewer2D::DispSetup dispsu )
{
    uiODViewer2D* vwr2d = new uiODViewer2D( appl_, probe, dispsu );
    vwr2d->setUpView( curlayid );
    viewers_ += vwr2d;
    attachNotifiersAndSetAuxData( vwr2d );
    return vwr2d->viewerObjID();
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
    const float x1eps  = mCast(float,wperpixel.x_) * sEPSPixWidth; \
    const int x1auxposidx = \
	curvwr.appearance().annot_.x1_.auxPosIdx( mCast(float,wp.x_), x1eps ); \
    const float x2eps  = mCast(float,wperpixel.y_) * sEPSPixWidth; \
    const int x2auxposidx = \
	curvwr.appearance().annot_.x2_.auxPosIdx( mCast(float,wp.y_), x2eps );

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
	    if ( !mIsUdfGeomID(curvwr2d->geomID()) )
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
	TypeSet<OD::PlotAnnotation>& xauxannot =
	    selauxannot_.isx1_ ? curvwr.appearance().annot_.x1_.auxannot_
			     : curvwr.appearance().annot_.x2_.auxannot_;
	if ( !xauxannot.validIdx(selauxannot_.auxposidx_) )
	    return;

	OD::PlotAnnotation& selauxannot = xauxannot[selauxannot_.auxposidx_];
	if ( selauxannot.isNormal() )
	    return;

	const StepInterval<double> xrg =
	    curvwr2d->viewwin()->viewer().posRange( selauxannot_.isx1_ );
	const int newposidx = xrg.nearestIndex( selauxannot_.isx1_
						? wp.x_ : wp.y_);
	const float newpos = mCast(float, xrg.atIndex(newposidx) );
	selauxannot.pos_ = newpos;
	auto vwr2ddir = curvwr2d->getTrcKeyZSampling().defaultDir();
	if ( (vwr2ddir==OD::InlineSlice && selauxannot_.isx1_) ||
	     (vwr2ddir==OD::ZSlice && !selauxannot_.isx1_) )
	    selauxannot.txt_ = toUiString( "%1 %2" )
				.arg(uiStrings::sInl())
				.arg( mNINT32(newpos) );
	else if ( (vwr2ddir==OD::CrosslineSlice && selauxannot_.isx1_) ||
		  (vwr2ddir==OD::ZSlice && selauxannot_.isx1_) )
	    selauxannot.txt_ = toUiString( "%1 %2" )
				.arg(uiStrings::sCrl())
				.arg( mNINT32(newpos) );
	else if ( (vwr2ddir==OD::InlineSlice && !selauxannot_.isx1_) ||
		  (vwr2ddir==OD::CrosslineSlice && !selauxannot_.isx1_) )
	    selauxannot.txt_ = toUiString( "%1 %2" )
				.arg(uiStrings::sZ())
				.arg( newpos*curvwr2d->zDomain().userFactor() );
    }

    setAuxAnnotLineStyles( curvwr, true );
    setAuxAnnotLineStyles( curvwr, false );
    curvwr.handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAuxAnnotLineStyles( uiFlatViewer& vwr, bool forx1 )
{
    FlatView::Annotation& annot = vwr.appearance().annot_;
    TypeSet<OD::PlotAnnotation>& auxannot = forx1 ? annot.x1_.auxannot_
					      : annot.x2_.auxannot_;
    for ( int idx=0; idx<auxannot.size(); idx++ )
	if ( !auxannot[idx].isNormal() )
	    auxannot[idx].linetype_ = OD::PlotAnnotation::Bold;

    const int selannotidx = selauxannot_.auxposidx_;
    if ( !(forx1^selauxannot_.isx1_) && auxannot.validIdx(selannotidx) )
	if ( !auxannot[selannotidx].isNormal() )
	    auxannot[selannotidx].linetype_ = OD::PlotAnnotation::HighLighted;
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
    TypeSet<OD::PlotAnnotation>& auxannot =
	selauxannot_.isx1_ ? vwr.appearance().annot_.x1_.auxannot_
			   : vwr.appearance().annot_.x2_.auxannot_;
    const TrcKeyZSampling& tkzs = vwr2d->getTrcKeyZSampling();
    const int selannotidx = selauxannot_.auxposidx_;
    if ( !tkzs.isFlat() || !auxannot.validIdx(selannotidx) )
	return;

    uiODViewer2D* clickedvwr2d = 0;
    if ( tkzs.hsamp_.is2D() )
    {
	if ( auxannot[selannotidx].isNormal() )
	    return;

	Line2DInterSection::Point intpoint2d( mUdfGeomID, mUdfGeomID,
					      mUdf(int), mUdf(int));
	const float auxpos = auxannot[selannotidx].pos_;
	intpoint2d = intersectingLineID( vwr2d, auxpos );
	if ( mIsUdfGeomID(intpoint2d.otherid_) )
	   return;
	clickedvwr2d = find2DViewer( intpoint2d.otherid_ );
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

	if ( tkzs.defaultDir()!=OD::InlineSlice && selauxannot_.isx1_ )
	{
	    const int auxpos = mNINT32(selauxannot_.oldauxpos_);
	    const int newauxpos = mNINT32(auxannot[selannotidx].pos_);
	    oldtkzs.hsamp_.setLineRange( Interval<int>(auxpos,auxpos) );
	    newtkzs.hsamp_.setLineRange( Interval<int>(newauxpos,newauxpos) );
	}
	else if ( tkzs.defaultDir()!=OD::ZSlice && !selauxannot_.isx1_ )
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
	    clickedvwr2d->getProbe().setPos( newtkzs );
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

    mGetAuxAnnotIdx

    float samplecrdz = mCast(float,coord.z_);
    SI().snapZ( samplecrdz );
    if ( meh->event().leftButton() )
    {
	if ( curvwr.appearance().annot_.editable_
	  || !mIsUdfGeomID(curvwr2d->geomID())
	  || (x1auxposidx<0 && x2auxposidx<0))
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
    Line2DInterSection::Point intpoint2d( mUdfGeomID, mUdfGeomID,
					  mUdf(int), mUdf(int) );
    const TrcKeyZSampling& tkzs = curvwr2d->getTrcKeyZSampling();
    if ( tkzs.hsamp_.is2D() )
    {
	if ( x1auxposidx>=0 &&
	     curvwr.appearance().annot_.x1_.auxannot_[x1auxposidx].isNormal() )
	{
	    intpoint2d = intersectingLineID( curvwr2d, mCast(float,wp.x_) );
	    if ( mIsUdfGeomID(intpoint2d.otherid_) )
	       return;
	    const uiString show2dtxt = m3Dots(tr("Show Line '%1'")).arg(
				intpoint2d.otherid_.name() );
	    menu.insertAction( new uiAction(show2dtxt), 0 );
	}
    }
    else
    {
	const BinID bid = SI().transform( coord.getXY() );
	const uiString showinltxt
			= m3Dots(tr("Show In-line %1")).arg( bid.inl() );
	const uiString showcrltxt
			= m3Dots(tr("Show Cross-line %1")).arg( bid.crl());
	const uiString showztxt = m3Dots(tr("Show Z-slice %1"))
		.arg( mNINT32(samplecrdz*curvwr2d->zDomain().userFactor()) );

	const bool isflat = tkzs.isFlat();
	const auto dir = tkzs.defaultDir();
	if ( !isflat || dir!=OD::InlineSlice )
	    menu.insertAction( new uiAction(showinltxt), 1 );
	if ( !isflat || dir!=OD::CrosslineSlice )
	    menu.insertAction( new uiAction(showcrltxt), 2 );
	if ( !isflat || dir!=OD::ZSlice )
	    menu.insertAction( new uiAction(showztxt), 3 );
    }

    menu.insertAction( new uiAction(m3Dots(uiStrings::sProperties())), 4 );

    const int menuid = menu.exec();
    if ( menuid>=0 && menuid<4 )
    {
	const BinID bid = SI().transform( coord .getXY() );
	uiWorldPoint initialcentre( uiWorldPoint::udf() );
	TrcKeyZSampling newtkzs( OD::UsrWork );
	newtkzs.hsamp_.setGeomSystem( tkzs.hsamp_.geomSystem() );
	ConstRefMan<ZAxisTransform> zat = curvwr2d->getZAxisTransform();
	if ( zat )
	{
	    newtkzs.zsamp_ = zat->getZInterval( false );
	    newtkzs.zsamp_.step = zat->getGoodZStep();
	}

	if ( menuid==0 )
	{
	    const auto& geom2d = SurvGeom::get2D( intpoint2d.otherid_ );
	    const PosInfo::Line2DData& l2ddata = geom2d.data();
	    const StepInterval<int> trcnrrg = l2ddata.trcNrRange();
	    const float trcdist =
		l2ddata.distBetween( trcnrrg.start, intpoint2d.othertrcnr_ );
	    if ( mIsUdf(trcdist) )
		return;
	    initialcentre = uiWorldPoint( mCast(double,trcdist), samplecrdz );
	    newtkzs.hsamp_.setTo( intpoint2d.otherid_ );
	    const auto lnr = intpoint2d.otherid_.lineNr();
	    newtkzs.hsamp_.setLineRange( Interval<int>(lnr,lnr) );
	}
	else if ( menuid == 1 )
	{
	    newtkzs.hsamp_.setLineRange( Interval<int>(bid.inl(),bid.inl()) );
	    initialcentre = uiWorldPoint( mCast(double,bid.crl()), samplecrdz );
	}
	else if ( menuid == 2 )
	{
	    newtkzs.hsamp_.setTrcRange( Interval<int>(bid.crl(),bid.crl()) );
	    initialcentre = uiWorldPoint( mCast(double,bid.inl()), samplecrdz );
	}
	else if ( menuid == 3 )
	{
	    newtkzs.zsamp_ = Interval<float>( mCast(float,samplecrdz),
					      mCast(float,samplecrdz) );
	    initialcentre = uiWorldPoint( mCast(double,bid.inl()),
					  mCast(double,bid.crl()) );
	}

	Probe* probe = createNewProbe( newtkzs );
	if ( !probe )
	    return;

	fillProbeFromExisting( *probe, *curvwr2d );

	uiODViewer2D::DispSetup su;
	su.initialcentre_ = initialcentre;
	const uiFlatViewStdControl* control = curvwr2d->viewControl();
	su.initialx1pospercm_ = control->getCurrentPosPerCM( true );
	if ( newtkzs.defaultDir()!=OD::ZSlice && curvwr2d->isVertical())
	    su.initialx2pospercm_ = control->getCurrentPosPerCM( false );

	displayIn2DViewer( *probe, ProbeLayer::ID::getInvalid(), su );
    }
    else if ( menuid == 4 )
	curvwr2d->viewControl()->doPropertiesDialog( 0 );
}


Probe* uiODViewer2DMgr::createNewProbe( const TrcKeyZSampling& pos ) const
{
    Probe* newprobe = 0;
    if ( pos.hsamp_.is2D() )
    {
	Line2DProbe* l2dprobe = new Line2DProbe();
	l2dprobe->setGeomID( pos.hsamp_.getGeomID() );
	newprobe = l2dprobe;
    }
    else if ( pos.defaultDir()==OD::InlineSlice )
	newprobe = new InlineProbe();
    else if ( pos.defaultDir()==OD::CrosslineSlice )
	newprobe = new CrosslineProbe();
    else
	newprobe = new ZSliceProbe();
    newprobe->setPos( pos );
    SilentTaskRunnerProvider trprov;
    ProbeMGR().store( *newprobe, trprov );
    return newprobe;
}


void uiODViewer2DMgr::fillProbeFromExisting( Probe& probe,
					     const uiODViewer2D& vwr2d ) const
{
    for ( int idx=0; idx<1; idx++ )
    {
	const bool iswva = idx;
	AttribProbeLayer* attrlayer = new AttribProbeLayer();
	attrlayer->setSelSpec( vwr2d.selSpec(iswva) );
	uiFlatViewer& vwr = const_cast<uiFlatViewer&>(
				vwr2d.viewwin()->viewer(0) );
	const ColTab::Mapper& mapper =
		*(iswva ? vwr.appearance().ddpars_.wva_.mapper_
			: vwr.appearance().ddpars_.vd_.mapper_);
	if ( !iswva )
	    attrlayer->setSequence( *ColTab::SeqMGR().getAny(
				vwr.appearance().ddpars_.vd_.colseqname_) );
	attrlayer->mapper() = mapper;
	probe.addLayer( attrlayer );
    }
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
	if ( geom2dids_.size()==0 )
	{
	    BufferStringSet lnms;
	    SeisIOObjInfo::getLinesWithData( lnms, geom2dids_ );
	}
	if ( geom2dids_.size()==0 ) return;

	l2dintersections_ = new Line2DInterSectionSet;
	BendPointFinder2DGeomSet bpfinder( geom2dids_ );
	bpfinder.execute();
	Line2DInterSectionFinder intfinder( bpfinder.bendPoints(),
					    *l2dintersections_ );
	intfinder.execute();
    }
}



uiODViewer2D* uiODViewer2DMgr::getParent2DViewer( int vwr2dobjid )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	ObjectSet<Vw2DDataObject> vw2dobjs;
	uiODViewer2D* viewer2d = getViewer2D( idx );
	viewer2d->dataMgr()->getObjects( vw2dobjs );
	for ( int objidx=0; objidx<vw2dobjs.size(); objidx++ )
	{
	    if ( vw2dobjs[objidx]->id()==vwr2dobjid )
		return viewer2d;
	}
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( ViewerObjID id )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODViewer2D* viewer2d = getViewer2D( idx );
	const ViewerObjID vwrid = viewer2d->viewerObjID();
	if ( vwrid == id )
	    return viewer2d;
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const MouseEventHandler& meh )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODViewer2D* viewer2d = getViewer2D( idx );
	if ( viewer2d->viewwin() &&
	    viewer2d->viewControl()->getViewerIdx(&meh,true) != -1 )
	    return viewer2d;
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const Pos::GeomID& geomid )
{
    if ( mIsUdfGeomID(geomid) )
	return 0;

    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODViewer2D* viewer2d = getViewer2D( idx );
	if ( viewer2d->geomID() == geomid )
	    return viewer2d;
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isFlat() )
	return 0;

    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODViewer2D* viewer2d = getViewer2D( idx );
	if ( viewer2d->getTrcKeyZSampling() == tkzs )
	    return viewer2d;
    }

    return 0;
}


void uiODViewer2DMgr::getVWR2DDataGeomIDs( const uiODViewer2D* vwr2d,
					   GeomIDSet& commongids ) const
{
    commongids.erase();
    if ( mIsUdfGeomID(vwr2d->geomID()) )
	return;

    const Attrib::DescSet& ads2d = Attrib::DescSet::global( true );
    const Attrib::Desc* wvadesc =
	ads2d.getDesc( vwr2d->selSpec(true).id() );
    const Attrib::Desc* vddesc =
	ads2d.getDesc( vwr2d->selSpec(false).id() );
    if ( !wvadesc && !vddesc )
	return;

    const DBKey wvaid( wvadesc ? wvadesc->getStoredID(true)
				 : vddesc->getStoredID(true) );
    const DBKey vdmid( vddesc ? vddesc->getStoredID(true)
				: wvadesc->getStoredID(true) );
    const SeisIOObjInfo wvasi( wvaid );
    const SeisIOObjInfo vdsi( vdmid );
    BufferStringSet wvalnms, vdlnms;
    wvasi.getLineNames( wvalnms );
    vdsi.getLineNames( vdlnms );

    for ( int lidx=0; lidx<wvalnms.size(); lidx++ )
    {
	const char* wvalnm = wvalnms.get(lidx).buf();
	if ( vdlnms.isPresent(wvalnm) )
	    commongids += SurvGeom::getGeomID( wvalnm );
    }
}


void uiODViewer2DMgr::setVWR2DIntersectionPositions( uiODViewer2D* vwr2d )
{
    const TrcKeyZSampling& tkzs = vwr2d->getTrcKeyZSampling();
    if ( !tkzs.isFlat() ) return;

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer( 0 );
    FlatView::Annotation& annot = vwr.appearance().annot_;
    TypeSet<OD::PlotAnnotation>& x1auxannot = annot.x1_.auxannot_;
    TypeSet<OD::PlotAnnotation>& x2auxannot = annot.x2_.auxannot_;
    x1auxannot.erase(); x2auxannot.erase();

    if ( !mIsUdfGeomID(vwr2d->geomID()) )
    {
	reCalc2DIntersetionIfNeeded( vwr2d->geomID() );
	const int intscidx = intersection2DIdx( vwr2d->geomID() );
	if ( intscidx<0 )
	    return;
	const Line2DInterSection* intsect = (*l2dintersections_)[intscidx];
	if ( !intsect )
	    return;

	GeomIDSet datagids;
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
	    if ( !datagids.isPresent(intpos.otherid_) )
		continue;

	    OD::PlotAnnotation newannot;
	    const uiODViewer2D* curvwr2d = find2DViewer( intpos.otherid_ );
	    if ( curvwr2d &&
		    curvwr2d->getZAxisTransform()==vwr2d->getZAxisTransform() )
		newannot.linetype_ = OD::PlotAnnotation::Bold;

	    const int posidx = trcrg.getIndex( intpos.mytrcnr_ );
	    newannot.pos_ = mCast(float,posdata->position(true,posidx));
	    newannot.txt_ = toUiString( intpos.otherid_.name() );
	    x1auxannot += newannot;
	}
    }
    else
    {
	const auto dir = tkzs.defaultDir();
	for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
	{
	    uiODViewer2D* curvwr2d = getViewer2D( vwridx );
	    const TrcKeyZSampling& idxvwrtkzs = curvwr2d->getTrcKeyZSampling();
	    const auto idxvwrdir = idxvwrtkzs.defaultDir();
	    if ( curvwr2d==vwr2d || idxvwrdir==dir || !idxvwrtkzs.isFlat() ||
		    curvwr2d->getZAxisTransform()!=vwr2d->getZAxisTransform() )
		continue;

	    BinID intersecbid = BinID::udf();
	    OD::PlotAnnotation newannot;
	    newannot.linetype_ = OD::PlotAnnotation::Bold;

	    if ( dir == OD::InlineSlice )
	    {
		if ( idxvwrdir==OD::CrosslineSlice )
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
	    else if ( dir == OD::CrosslineSlice )
	    {
		if ( idxvwrdir==OD::InlineSlice )
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
		if ( idxvwrdir==OD::InlineSlice )
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
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	setVWR2DIntersectionPositions( vwr2d );
    }

    DBKeySet hor3dids;
    getLoadedHorizon3Ds( hor3dids );
    for ( int idx=0; idx<hor3dids.size(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,
			EM::Hor3DMan().getObject(hor3dids[idx]))
	if ( !hor3d )
	    continue;
	EM::ObjectCallbackData cbdata( EM::Object::cAttribChange(),
				Monitorable::ChangeData::cUnspecChgID() );
	hor3d->sendEMNotifFromOutside( cbdata );
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
    Line2DInterSection::Point udfintpoint( mUdfGeomID, mUdfGeomID,
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
    GeomIDSet datagids;
    getVWR2DDataGeomIDs( vwr2d, datagids );
    for ( int idx=0; idx<int2d->size(); idx++ )
    {
	const Line2DInterSection::Point& intpoint = int2d->getPoint( idx );
	if ( !datagids.isPresent(intpoint.otherid_) )
	    continue;

	const double diff = Math::Abs( (double)(intpoint.mytrcnr_-trcnrd) );
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
	remove2DViewer( vwr2d->viewerObjID() );
}


void uiODViewer2DMgr::remove2DViewer( ViewerObjID id )
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( idx );
	if ( vwr2d->viewerObjID() != id )
	    continue;

	mDetachCB( vwr2d->dataMgr()->dataObjAdded,
		   uiODViewer2DMgr::viewObjAdded );
	mDetachCB( vwr2d->dataMgr()->dataObjToBeRemoved,
		   uiODViewer2DMgr::viewObjToBeRemoved );
	auto* vwr = viewers_.removeSingle( idx );
	vwr->detachAllNotifiers();
	setAllIntersectionPositions();
	OBJDISP()->go( vwr );
	return;
    }
}


void uiODViewer2DMgr::viewObjAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,vw2dobjid,cb);
    vw2dObjAdded.trigger( vw2dobjid );
}


void uiODViewer2DMgr::viewObjToBeRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(int,vw2dobjid,cb);
    vw2dObjToBeRemoved.trigger( vw2dobjid );
}

//TODO PrIMPL replace fillPar/usePar handling via Probe
void uiODViewer2DMgr::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( idx );
	if ( !vwr2d->viewwin() ) continue;

	IOPar vwrpar;
	//TODO PrIMPL replace vwrpar.set( sKeyVisID(), vwr2d->visid_ );
	bool wva = vwr2d->viewwin()->viewer().appearance().ddpars_.wva_.show_;
	vwrpar.setYN( sKeyWVA(), wva );
	vwrpar.set( sKeyAttrID(), vwr2d->selSpec(wva).id() );
	vwr2d->fillPar( vwrpar );

	iop.mergeComp( vwrpar, toString( idx ) );
    }
}


void uiODViewer2DMgr::usePar( const IOPar& iop )
{
    deepErase( viewers_ );

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> vwrpar = iop.subselect( toString(idx) );
	if ( !vwrpar || !vwrpar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}
	/*int visid; bool wva; int attrid;
	    TODO PrIMPL replace with Probe controlled things
	if ( vwrpar->get( sKeyVisID(), visid ) &&
	     vwrpar->get( sKeyAttrID(), attrid ) &&
	     vwrpar->getYN( sKeyWVA(), wva ) )
	{
	    const int nrattribs = visServ().getNrAttribs( visid );
	    const int attrnr = nrattribs-1;
	    ViewerObjID vwrid = displayIn2DViewer( visid, attrnr, wva );
	    uiODViewer2D* curvwr = find2DViewer( vwrid );
	    if ( curvwr ) curvwr->usePar( *vwrpar );
	}*/
    }
}


void uiODViewer2DMgr::getVwr2DObjIDs( TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getVwr2DObjIDs( vw2dobjids );
    }
}


void uiODViewer2DMgr::getHor3DVwr2DIDs( const DBKey& emid,
					TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getHor3DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2DMgr::removeHorizon3D( const DBKey& emid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removeHorizon3D( emid );
    }
}


void uiODViewer2DMgr::getLoadedHorizon3Ds( DBKeySet& emids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedHorizon3Ds( emids );
    }
}


void uiODViewer2DMgr::addHorizon3Ds( const DBKeySet& emids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addHorizon3Ds( emids );
    }
}


void uiODViewer2DMgr::addNewTrackingHorizon3D( const DBKey& emid )
{
    addNewTrackingHorizon3D( emid, -1 );
}


void uiODViewer2DMgr::addNewTrackingHorizon3D( const DBKey& emid, int sceneid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addNewTrackingHorizon3D( emid );
    }

    DBKeySet emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon3D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getHor2DVwr2DIDs( const DBKey& emid,
					TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getHor2DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2DMgr::removeHorizon2D( const DBKey& emid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removeHorizon2D( emid );
    }
}


void uiODViewer2DMgr::getLoadedHorizon2Ds( DBKeySet& emids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedHorizon2Ds( emids );
    }
}


void uiODViewer2DMgr::addHorizon2Ds( const DBKeySet& emids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addHorizon2Ds( emids );
    }
}


void uiODViewer2DMgr::addNewTrackingHorizon2D( const DBKey& emid )
{
    addNewTrackingHorizon2D( emid, -1 );
}


void uiODViewer2DMgr::addNewTrackingHorizon2D( const DBKey& emid, int sceneid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addNewTrackingHorizon2D( emid );
    }

    DBKeySet emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Horizon2D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::removeFault( const DBKey& emid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removeFault( emid );
    }
}


void uiODViewer2DMgr::getLoadedFaults( DBKeySet& emids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedFaults( emids );
    }
}


void uiODViewer2DMgr::addFaults( const DBKeySet& emids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addFaults( emids );
    }
}


void uiODViewer2DMgr::addNewTempFault( const DBKey& emid )
{
    addNewTempFault( emid, -1 );
}


void uiODViewer2DMgr::addNewTempFault( const DBKey& emid, int sceneid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addNewTempFault( emid );
    }

    DBKeySet emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::Fault3D::typeStr(), sceneid );
    if ( emids.isPresent(emid) )
	return;

    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getFaultSSVwr2DIDs( const DBKey& emid,
					TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getFaultSSVwr2DIDs( emid, vw2dobjids );
    }
}



void uiODViewer2DMgr::removeFaultSS( const DBKey& emid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removeFaultSS( emid );
    }
}


void uiODViewer2DMgr::getLoadedFaultSSs( DBKeySet& emids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedFaultSSs( emids );
    }
}


void uiODViewer2DMgr::addFaultSSs( const DBKeySet& emids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addFaultSSs( emids );
    }
}


void uiODViewer2DMgr::addNewTempFaultSS( const DBKey& emid )
{
    addNewTempFaultSS( emid, -1 );
}


void uiODViewer2DMgr::addNewTempFaultSS( const DBKey& emid, int sceneid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addNewTempFaultSS( emid );
    }

    DBKeySet emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(),
				     sceneid );
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid, sceneid );
}


void uiODViewer2DMgr::getFaultSS2DVwr2DIDs( const DBKey& emid,
					TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getFaultSS2DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2DMgr::removeFaultSS2D( const DBKey& emid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removeFaultSS2D( emid );
    }
}


void uiODViewer2DMgr::getLoadedFaultSS2Ds( DBKeySet& emids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedFaultSS2Ds( emids );
    }
}


void uiODViewer2DMgr::addFaultSS2Ds( const DBKeySet& emids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addFaultSS2Ds( emids );
    }
}


void uiODViewer2DMgr::addNewTempFaultSS2D( const DBKey& emid )
{
    addNewTempFaultSS2D( emid, -1 );
}


void uiODViewer2DMgr::addNewTempFaultSS2D( const DBKey& emid, int sceneid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addNewTempFaultSS2D( emid );
    }

    DBKeySet emids;
    appl_.sceneMgr().getLoadedEMIDs( emids, EM::FaultStickSet::typeStr(),
				     sceneid );
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid, sceneid );
}



void uiODViewer2DMgr::getPickSetVwr2DIDs( const DBKey& mid,
					  TypeSet<int>& vw2dobjids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getPickSetVwr2DIDs( mid, vw2dobjids );
    }
}


void uiODViewer2DMgr::removePickSet( const DBKey& mid )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->removePickSet( mid );
    }
}


void uiODViewer2DMgr::getLoadedPickSets( DBKeySet& mids ) const
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	const uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->getLoadedPickSets( mids );
    }
}


void uiODViewer2DMgr::addPickSets( const DBKeySet& mids )
{
    for ( int vwridx=0; vwridx<viewers_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = getViewer2D( vwridx );
	vwr2d->addPickSets( mids );
    }
}


void uiODViewer2DMgr::handleRequest( ViewerID originvwrid,
				     RequestType req,
				     const IOPar& prinfopar )
{
    PtrMan<Presentation::ObjInfo> prinfo = OD::PrIFac().create( prinfopar );
    if ( !prinfo )
	return;

    FixedString probeprinfpkey( ProbePresentationInfo::sFactoryKey() );
    if ( probeprinfpkey == prinfo->objTypeKey() )
	return;

    VwrTypeMgr::handleRequest( originvwrid, req, prinfopar );
}
