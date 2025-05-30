/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visfaultsticksetdisplay.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "iopar.h"
#include "keystrs.h"
#include "limits.h"
#include "mpeengine.h"
#include "undo.h"
#include "viscoord.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vissurvobj.h"
#include "vistransform.h"
#include "zdomain.h"

namespace visSurvey
{

const char* FaultStickSetDisplay::sKeyEarthModelID()	{ return "EM ID"; }
const char* FaultStickSetDisplay::sKeyDisplayOnlyAtSections()
					{ return "Display only at sections"; }

#define mDefaultMarkerSize 3
#define mSceneIdx (scene_ ? scene_->fixedIdx() : -1)

FaultStickSetDisplay::FaultStickSetDisplay()
    : visBase::VisualObjectImpl(true)
    , StickSetDisplay(true)
    , colorchange(this)
    , displaymodechange(this)
{
    ref();
    sticks_ = visBase::Lines::create();
    activestick_ = visBase::Lines::create();

    RefMan<visBase::DrawStyle> drawstyle = visBase::DrawStyle::create();
    stickdrawstyle_ = sticks_->addNodeState( drawstyle.ptr() );

    OD::LineStyle stickls( OD::LineStyle::Solid, 3 );
    stickdrawstyle_->setLineStyle( stickls );
    addChild( sticks_->osgNode() );

    RefMan<visBase::DrawStyle> actdrawstyle = visBase::DrawStyle::create();
    activestickdrawstyle_ = activestick_->addNodeState( actdrawstyle.ptr() );

    stickls.width_ += 2;
    activestickdrawstyle_->setLineStyle( stickls );
    addChild( activestick_->osgNode() );

    for ( int idx=0; idx<3; idx++ )
    {
	RefMan<visBase::MarkerSet> markerset = visBase::MarkerSet::create();
	addChild( markerset->osgNode() );
	markerset->setMarkersSingleColor( idx ? OD::Color(0,255,0) :
						OD::Color(255,0,255) );
	knotmarkersets_ += markerset.ptr();
    }

    activestick_->setPickable( true );
    activestick_->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				   false );
    sticks_->setPickable( true );
    sticks_->enableTraversal( visBase::cDraggerIntersecTraversalMask(), false );
    setCurScene( scene_ );
    unRefNoDelete();
}


FaultStickSetDisplay::~FaultStickSetDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( nullptr );
    if ( fsseditor_ )
	fsseditor_->removeUser();

    if ( fault_ )
	mDetachCB( fault_->change, FaultStickSetDisplay::emChangeCB );

    sticks_->removeNodeState( stickdrawstyle_.ptr() );
    activestick_->removeNodeState( activestickdrawstyle_.ptr() );
}


bool FaultStickSetDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    mDetachCB( *zaxistransform_->changeNotifier(),
		       FaultStickSetDisplay::dataTransformCB );
	if ( voiid_>0 )
	{
	    zaxistransform_->removeVolumeOfInterest( voiid_ );
	    voiid_ = -1;
	}
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    mAttachCB( *zaxistransform_->changeNotifier(),
		       FaultStickSetDisplay::dataTransformCB );
    }

    return true;
}


const ZAxisTransform* FaultStickSetDisplay::getZAxisTransform() const
{
    return zaxistransform_.ptr();
}


void FaultStickSetDisplay::dataTransformCB( CallBacker* )
{
    // TODO: implement
}


void FaultStickSetDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
	mDetachCB( eventcatcher_->eventhappened, FaultStickSetDisplay::mouseCB);

    eventcatcher_ = vec;

    if ( eventcatcher_ )
	mAttachCB( eventcatcher_->eventhappened, FaultStickSetDisplay::mouseCB);

    if ( viseditor_ )
	viseditor_->setSceneEventCatcher( eventcatcher_.ptr() );
}


void FaultStickSetDisplay::setScene( Scene* scene )
{
    SurveyObject::setScene( scene );
    if ( fsseditor_ )
	fsseditor_->setSceneIdx( mSceneIdx );
}


EM::ObjectID FaultStickSetDisplay::getEMObjectID() const
{
    return fault_ ? fault_->id() : EM::ObjectID::udf();
}


RefMan<MPE::ObjectEditor> FaultStickSetDisplay::getMPEEditor( bool create )
{
    if ( !create )
	return fsseditor_.ptr();

    const EM::ObjectID emid = getEMObjectID();
    if ( MPE::engine().hasEditor(emid) )
    {
	RefMan<MPE::ObjectEditor> objeditor =
				    MPE::engine().getEditorByID( emid );
	fsseditor_ = dynamic_cast<MPE::FaultStickSetEditor*>( objeditor.ptr() );
    }

    if ( !fsseditor_ )
    {
	const EM::FaultStickSet* emfaultstickset = emFaultStickSet();
	if ( !emfaultstickset )
	    return nullptr;

	fsseditor_ = MPE::FaultStickSetEditor::create( *emfaultstickset );
	if ( fsseditor_ )
            MPE::engine().addEditor( *fsseditor_.ptr() );
    }

    return fsseditor_.ptr();
}


#define mSetStickIntersectPointColor( color ) \
     knotmarkersets_[2]->setMarkersSingleColor( color );

#define mErrRet(s) { errmsg_ = s; return false; }

bool FaultStickSetDisplay::setEMObjectID( const EM::ObjectID& emid )
{
    if ( viseditor_ )
	viseditor_->setEditor( nullptr );

    if ( fsseditor_ )
    {
	fsseditor_->setEditIDs( nullptr );
	fsseditor_->removeUser();
    }

    fsseditor_ = nullptr;
    if ( fault_ )
	mDetachCB( fault_->change, FaultStickSetDisplay::emChangeCB );

    fault_ = nullptr;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return false;

    fault_ = sCast(EM::Fault*,emfss);
    if ( !emfss->name().isEmpty() )
    {
	setName( emfss->name() );
	sticks_->setName( emfss->name() );
    }

    hideallknots_ = !fault_->isEmpty();

    if ( !viseditor_ )
    {
	viseditor_ = MPEEditor::create();
	viseditor_->setSceneEventCatcher( eventcatcher_.ptr() );
	viseditor_->setDisplayTransformation( displaytransform_.ptr() );
	viseditor_->sower().alternateSowingOrder();
	viseditor_->sower().setIfDragInvertMask();
	addChild( viseditor_->osgNode() );
	mAttachCB( viseditor_->draggingStarted,
				    FaultStickSetDisplay::draggingStartedCB );
    }

    RefMan<MPE::ObjectEditor> fsseditor = getMPEEditor( true );
    if ( fsseditor_ )
    {
	fsseditor->addUser();
	fsseditor_->setSceneIdx( mSceneIdx );
	fsseditor_->setEditIDs( &editpids_ );
    }

    viseditor_->setEditor( fsseditor_.ptr() );
    mAttachCB( viseditor_->sower().sowingend,
	       FaultStickSetDisplay::sowingFinishedCB );

    getMaterial()->setColor( fault_->preferredColor() );
    viseditor_->setMarkerSize( mDefaultMarkerSize );
    mSetStickIntersectPointColor( fault_->preferredColor() );
    setPreferedMarkerStyle( fault_->getPosAttrMarkerStyle(0) );

    updateSticks();
    updateKnotMarkers();
    updateManipulator();
    mAttachCB( fault_->change, FaultStickSetDisplay::emChangeCB );

    return true;
}


MultiID FaultStickSetDisplay::getMultiID() const
{
    return fault_ ? fault_->multiID() : MultiID();
}


void FaultStickSetDisplay::setColor( OD::Color nc )
{
    if ( fault_ )
	fault_->setPreferredColor( nc );
    else
	getMaterial()->setColor( nc );

    colorchange.trigger();
}


NotifierAccess* FaultStickSetDisplay::materialChange()
{
    return &getMaterial()->change;
}


OD::Color FaultStickSetDisplay::getColor() const
{
    return getMaterial()->getColor();
}


const OD::LineStyle* FaultStickSetDisplay::lineStyle() const
{ return &stickdrawstyle_->lineStyle(); }


void FaultStickSetDisplay::setLineStyle( const OD::LineStyle& ls )
{
    stickdrawstyle_->setLineStyle( ls );

    OD::LineStyle activestickls( ls );
    activestickls.width_ += 2;
    activestickdrawstyle_->setLineStyle( activestickls );

    requestSingleRedraw();
}


void FaultStickSetDisplay::setMarkerStyle( const MarkerStyle3D& ms )
{
    StickSetDisplay::setMarkerStyle( ms );
}


const MarkerStyle3D* FaultStickSetDisplay::markerStyle() const
{
    return StickSetDisplay::markerStyle();
}


void FaultStickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( viseditor_ )
	viseditor_->setDisplayTransformation( nt );

    sticks_->setDisplayTransformation( nt );
    activestick_->setDisplayTransformation( nt );

    StickSetDisplay::setDisplayTransformation( nt );
}


const mVisTrans* FaultStickSetDisplay::getDisplayTransformation() const
{
    return StickSetDisplay::getDisplayTransformation();
}


void FaultStickSetDisplay::updateEditPids()
{
    if ( !fault_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    editpids_.erase();

    const bool displayknots = !hideallknots_ && !stickselectmode_;
    if ( !displayknots )
	return;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,
		    fault_->geometryElement())
    if ( !fss || fss->isEmpty() )
	return;

    RowCol rc;
    const StepInterval<int> rowrg = fss->rowRange();
    for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_; rc.row()+=rowrg.step_ )
    {
	if ( fss->isStickHidden(rc.row(),mSceneIdx) )
	    continue;

	const StepInterval<int> colrg = fss->colRange( rc.row() );
        for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
              rc.col()+=colrg.step_ )
	{
	    if ( !fss->isKnotHidden(rc,mSceneIdx) )
		editpids_ += EM::PosID( fault_->id(), rc );
	}
    }

    if ( fsseditor_ )
	fsseditor_->editpositionchange.trigger();
}


static void addPolyLineCoordIdx( TypeSet<int>& coordidxlist, int idx )
{
    if ( coordidxlist.size()%2 )
    {
	if ( idx >= 0 )
	    { coordidxlist += idx; coordidxlist += idx; }
	else	// Negative index represents line break or end
	    coordidxlist.removeSingle( coordidxlist.size()-1 );
    }
    else if ( idx >= 0 )
	coordidxlist += idx;
}

static void addPolyLineCoordBreak( TypeSet<int>& coordidxlist )
{
    addPolyLineCoordIdx( coordidxlist, -1 );
}


EM::FaultStickSet* FaultStickSetDisplay::emFaultStickSet()
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,fault_.ptr());
    return emfss;
}


void FaultStickSetDisplay::updateSticks( bool activeonly )
{
    if ( !fault_ || !viseditor_ || viseditor_->sower().moreToSow() )
	return;

    visBase::Lines* poly = activeonly ? activestick_.ptr() : sticks_.ptr();

    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    fault_->geometryElement())
    int maxpos = (rcs->rowRange().nrSteps()+1)*(rcs->colRange().nrSteps()+1);

    poly->removeAllPrimitiveSets();
    Geometry::IndexedPrimitiveSet* primitiveset =
		    Geometry::IndexedPrimitiveSet::create( maxpos>USHRT_MAX );
    poly->addPrimitiveSet( primitiveset );

    if ( poly->getCoordinates()->size() )
	poly->getCoordinates()->setEmpty();

    TypeSet<int> coordidxlist;
    for ( int sidx=0; sidx<fault_->nrSections(); sidx++ )
    {
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 fault_->geometryElement())
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_;
							rc.row()+=rowrg.step_ )
	{
	    if ( activeonly && rc.row()!=activesticknr_ )
		continue;

	    if ( fss->isStickHidden(rc.row(),mSceneIdx) )
		continue;

	    Seis2DDisplay* s2dd = nullptr;
	    const EM::FaultStickSet* emfss = emFaultStickSet();
	    if ( emfss->geometry().pickedOn2DLine(rc.row()) )
	    {
		const Pos::GeomID geomid =
				emfss->geometry().pickedGeomID(rc.row());
		if ( !geomid.isUdf() )
		    s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	    }

	    const StepInterval<int> colrg = fss->colRange( rc.row() );

	    if ( !colrg.width() )
	    {
                rc.col() = colrg.start_;
		if ( isSelected() || fss->isKnotHidden(rc,mSceneIdx) )
		    continue;

		for ( int dim=0; dim<3; dim++ )
		{
		    const float step = dim==2 ? s3dgeom_->zStep()
					      : s3dgeom_->inlDistance();

		    for ( int dir=-1; dir<=1; dir+=2 )
		    {
			Coord3 pos = fss->getKnot( rc );
			pos[dim] += step * 0.5 * dir;
			const int ci = poly->getCoordinates()->addPos( pos );
			addPolyLineCoordIdx( coordidxlist, ci );
		    }
		    addPolyLineCoordBreak( coordidxlist );
		}
		continue;
	    }

            for ( rc.col()=colrg.start_; rc.col()<colrg.stop_;
                  rc.col()+=colrg.step_ )
	    {
		if ( fss->isKnotHidden(rc,mSceneIdx) )
		    continue;

		RowCol nextrc( rc );
                nextrc.col() += colrg.step_;

		if ( fss->isKnotHidden(nextrc,mSceneIdx) )
		{
		    addPolyLineCoordBreak( coordidxlist );
		    continue;
		}

		TypeSet<Coord3> coords;
		coords += fss->getKnot( rc );
		coords += fss->getKnot( nextrc );

		if ( s2dd )
		    s2dd->getLineSegmentProjection(coords[0],coords[1],coords);

		for ( int idx=0; idx<coords.size(); idx++ )
		{
		    if ( idx || coordidxlist.size()%2==0 )
		    {
			const Coord3& pos = coords[idx];
			const int ci = poly->getCoordinates()->addPos( pos );
			addPolyLineCoordIdx( coordidxlist, ci );
		    }
		}
	    }
	    addPolyLineCoordBreak( coordidxlist );
	}
    }

    if ( poly->getCoordinates()->size() )
    {
	primitiveset->append( coordidxlist.arr(), coordidxlist.size() );
	poly->dirtyCoordinates();
    }
    else
	poly->removeAllPrimitiveSets();

    if ( !activeonly )
	updateSticks( true );
}


bool FaultStickSetDisplay::isPicking() const
{
    return !stickselectmode_ && !locked_ && fault_ && fsseditor_ &&
	   viseditor_ && viseditor_->isOn() && isOn() && isSelected();
}


Coord3 FaultStickSetDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    scene_->getTempZStretchTransform()->transformBack( pos );
	if ( displaytransform_ )
	    displaytransform_->transformBack( pos );
    }
    return pos;
}


static float zdragoffset = 0.f;

#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getFixedZStretch()\
    : s3dgeom_->zScale() )\

#define mSetUserInteractionEnd() \
    if ( !viseditor_->sower().moreToSow() ) \
	EM::EMM().undo(fault_->id()).setUserInteractionEnd( \
		EM::EMM().undo(fault_->id()).currentEventID() );


void FaultStickSetDisplay::sowingFinishedCB( CallBacker* )
{
    makenewstick_ = true;
}


void FaultStickSetDisplay::mouseCB( CallBacker* cb )
{
    if ( stickselectmode_ )
    {
	stickSelectCB( cb );
	return;
    }

    if ( !fault_ || !fsseditor_ || !viseditor_ || !viseditor_->isOn() ||
	 !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack( const visBase::EventInfo&,eventinfo,cb);

    fsseditor_->setSowingPivot( disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;

    const EM::PosID mousepid =
		    viseditor_->mouseClickDragger( eventinfo.pickedobjids );

    EM::FaultStickSetGeometry& fssg = emFaultStickSet()->geometry();

    if ( eventinfo.buttonstate_ == OD::ControlButton )
    {
	makenewstick_ =  false;
	if ( !activestickid_.isUdf() )
	    fsseditor_->setLastClicked( activestickid_ );
	return;
    }

    if ( eventinfo.type == visBase::MouseDoubleClick )
    {
	makenewstick_ = true;
	return;
    }

    PlaneDataDisplay* plane = nullptr;
    Seis2DDisplay* s2dd = nullptr;
    RandomTrackDisplay* rdtd = nullptr;
    HorizonDisplay* hordisp = nullptr;
    const MultiID* pickedmid = nullptr;
    Pos::GeomID pickedgeomid;
    const char* pickednm = nullptr;
    PtrMan<Coord3> normal = nullptr;
    ConstPtrMan<MultiID> horid;
    BufferString horshiftname;
    Coord3 pos;

    if ( mousepid.isUdf() )
    {
	for ( const auto& visid : eventinfo.pickedobjids )
	{
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    if ( !dataobj )
		continue;

	    mDynamicCast(PlaneDataDisplay*,plane,dataobj);
	    if ( plane )
	    {
		pickedgeomid = Survey::default3DGeomID();
		normal = new Coord3( plane->getNormal(Coord3::udf()) );
		break;
	    }

	    mDynamicCast(RandomTrackDisplay*,rdtd,dataobj);
	    if ( rdtd )
	    {
		pickedgeomid = Survey::default3DGeomID();
		normal =
		    new Coord3( rdtd->getNormal(eventinfo.displaypickedpos) );
		break;
	    }

	    mDynamicCast(Seis2DDisplay*,s2dd,dataobj);
	    if ( s2dd )
	    {
		pickedgeomid = s2dd->getGeomID();
		break;
	    }

	    mDynamicCast(HorizonDisplay*,hordisp,dataobj);
	    if ( hordisp )
	    {
		pickedgeomid = Survey::default3DGeomID();
		horid = new MultiID( hordisp->getMultiID() );
		pickedmid = horid.ptr();
                horshiftname = hordisp->getTranslation().z_ *
				    scene_->zDomainInfo().userFactor();
		pickednm = horshiftname.buf();
		break;
	    }

	    mDynamicCastGet(FaultStickSetDisplay*,fssd,dataobj);
	    if ( fssd )
		return;
	}

	if ( !s2dd && !plane && !hordisp && !rdtd )
	{
	    setActiveStick( EM::PosID::udf() );
	    return;
	}

	pos = disp2world( eventinfo.displaypickedpos );
    }
    else
    {
	const int sticknr = mousepid.getRowCol().row();
	pos = fault_->getPos( mousepid );
	pickedmid = fssg.pickedMultiID( sticknr );
	pickednm = fssg.pickedName( sticknr );
	pickedgeomid = fssg.pickedGeomID( sticknr );
	zdragoffset = 0;
    }

    EM::PosID insertpid;
    fsseditor_->setZScale( mZScale() );
    fsseditor_->getInteractionInfo( insertpid, pickedmid, pickednm,
				    pickedgeomid, pos, normal.ptr());

    if ( mousepid.isUdf() && !viseditor_->isDragging() )
    {
	const EM::PosID pid =
	    fsseditor_->getNearestStick( pos,pickedgeomid,normal.ptr() );
	if ( !pid.isUdf() )
	{
	   setActiveStick( pid );
	   activestickid_ = pid;
	}
    }

    if ( locked_ || !pos.isDefined() ||
	 eventinfo.type!=visBase::MouseClick || viseditor_->isDragging() )
	return;

    if ( !mousepid.isUdf() )
    {
	fsseditor_->setLastClicked( mousepid );
	setActiveStick( mousepid );
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;


    if ( !mousepid.isUdf() && OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
    {
	// Remove knot/stick
	eventcatcher_->setHandled();
	if ( eventinfo.pressed )
	    return;

	editpids_.erase();
	const int rmnr = mousepid.getRowCol().row();
	if ( fssg.nrKnots(rmnr) == 1 )
	    fssg.removeStick( rmnr, true );
	else
	    fssg.removeKnot( mousepid.subID(), true);

	mSetUserInteractionEnd();
	updateEditPids();
	return;
    }

    if ( !mousepid.isUdf() || OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	return;

    if ( viseditor_->sower().activate(fault_->preferredColor(), eventinfo) )
	return;

    if ( eventinfo.pressed )
	return;

    if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) ||
	 insertpid.isUdf() || makenewstick_ )
    {
	// Add stick
	const Coord3 editnormal(
		    plane || rdtd ? *normal :
		    hordisp ? Coord3(0,0,1) :
		    Coord3(s2dd->getNormal(s2dd->getNearestTraceNr(pos)),0) );

	Geometry::FaultStickSet* fss = fssg.geometryElement();

	const int insertsticknr =
                !fss || fss->isEmpty() ? 0 : fss->rowRange().stop_+1;

	editpids_.erase();

	if ( pickedgeomid.isUdf() )
	    fssg.insertStick( insertsticknr, 0, pos, editnormal,
			      pickedmid, pickednm, true );
	else
	    fssg.insertStick( insertsticknr, 0, pos, editnormal,
			      pickedgeomid, true );

	const EM::SubID subid = RowCol(insertsticknr,0).toInt64();
	fsseditor_->setLastClicked( EM::PosID(fault_->id(),subid) );
	setActiveStick( EM::PosID(fault_->id(),subid) );
	mSetUserInteractionEnd();
	updateEditPids();
	makenewstick_ = false;
    }
    else
    {
	// Add knot
	editpids_.erase();
	fssg.insertKnot( insertpid.subID(), pos, true );
	fsseditor_->setLastClicked( insertpid );
	mSetUserInteractionEnd();
	updateEditPids();
    }

    eventcatcher_->setHandled();
}


void FaultStickSetDisplay::draggingStartedCB( CallBacker* )
{
    if ( !viseditor_ || !fsseditor_ )
	return;

    fsseditor_->setLastClicked( viseditor_->getActiveDragger() );
    setActiveStick( viseditor_->getActiveDragger() );
}


void FaultStickSetDisplay::stickSelectCB( CallBacker* cb )
{
    if ( !fault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    if ( getCurScene().ptr() != scene_ )
	setCurScene( scene_ );

    stickSelectionCB( cb, get3DSurvGeom() );
}


void FaultStickSetDisplay::setActiveStick( const EM::PosID& pid )
{
    if ( !viseditor_ )
	return;

    const bool allowactivestick = viseditor_->isOn() && !pid.isUdf();
    const int sticknr = allowactivestick ? pid.getRowCol().row() : mUdf(int);

    if ( activesticknr_ != sticknr )
    {
	activesticknr_ = sticknr;
	updateSticks( true );
    }
}


void FaultStickSetDisplay::emChangeCB( CallBacker* cber )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cber);
    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	getMaterial()->setColor( fault_->preferredColor() );
	mSetStickIntersectPointColor( fault_->preferredColor() );
    }

    EM::FaultStickSet* emfss = emFaultStickSet();
    if ( !emfss ) return;

    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange && emfss )
    {
	RowCol rc = cbdata.pid0.getRowCol();

	const MultiID* mid = emfss->geometry().pickedMultiID( rc.row() );
	const Pos::GeomID geomid = emfss->geometry().pickedGeomID( rc.row() );
	if ( !emfss->geometry().pickedOnPlane( rc.row()) )
	{
	    const char* nm = emfss->geometry().pickedName( rc.row() );
	    const Coord3 dragpos = emfss->getPos( cbdata.pid0 );
	    Coord3 pos = dragpos;

	    Seis2DDisplay* s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	    if ( s2dd )
		pos = s2dd->getNearestSubPos( pos, true );

	    HorizonDisplay* hordisp = mid ?
			HorizonDisplay::getHorizonDisplay( *mid ) : nullptr;
	    if ( hordisp )
	    {
		if ( displaytransform_ )
		    displaytransform_->transform( pos );

		const float dist = hordisp->calcDist( pos );
		if ( mIsUdf(dist) )
		{
		    pos = dragpos;
                    pos.z_ += zdragoffset;
		}
		else
		{
                    pos.z_ += dist;
                    pos.z_ -= hordisp->calcDist( pos );

		    if ( displaytransform_ )
			displaytransform_->transformBack( pos );
		    if ( nm )
			pos.z_ += toFloat(nm) /
					scene_->zDomainInfo().userFactor();

                    zdragoffset = (float) ( pos.z_ - dragpos.z_ );
		}
	    }

	    CallBack cb = mCB(this,FaultStickSetDisplay,emChangeCB);
	    emfss->change.remove( cb );
	    emfss->setPos( cbdata.pid0, pos, false );
	    emfss->change.notify( cb );
	}
    }

    if ( fault_ && !fault_->hasBurstAlert() )
	updateAll();
}


void FaultStickSetDisplay::showManipulator( bool yn )
{
    showmanipulator_ = yn;
    updateSticks();
    updateKnotMarkers();
    updateManipulator();
    displaymodechange.trigger();
}


void FaultStickSetDisplay::updateManipulator()
{
    const bool show = showmanipulator_ && !areAllKnotsHidden();
    if ( viseditor_ )
	viseditor_->turnOn( show && !stickselectmode_ );
}


void FaultStickSetDisplay::enableEditor( bool yn )
{
    if ( viseditor_ )
	viseditor_->turnOn( yn );
}


bool  FaultStickSetDisplay::isManipulatorShown() const
{
    return showmanipulator_;
}


bool FaultStickSetDisplay::removeSelections( TaskRunner* taskr )
{
    if ( !fault_ )
	return false;

    fault_->geometry().removeSelectedSticks( true );
    fault_->setChangedFlag();
    return SurveyObject::removeSelections( taskr );
}


void FaultStickSetDisplay::otherObjectsMoved(
				const ObjectSet<const SurveyObject>& /* objs */,
				const VisID& /* whichobj */ )
{
    if ( !displayonlyatsections_ )
	return;

    updateAll();
}


#define mHideKnotTolerance 2.0

bool FaultStickSetDisplay::coincidesWith2DLine(
			Geometry::FaultStickSet& fss, int sticknr,
			Pos::GeomID geomid )
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	 rowrg.snap(sticknr)!=sticknr )
	return res;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if ( !s2dd || !s2dd->isOn() || geomid!=s2dd->getGeomID() )
	    continue;

	const double onestepdist = Coord3(1,1,mZScale()).dot(
				s3dgeom_->oneStepTranslation(Coord3(0,0,1)) );

	bool curobjcoincides = false;
	TypeSet<int> showcols;
	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
							rc.col()+=colrg.step_ )
	{
	    Coord3 pos = fss.getKnot(rc);
	    if ( displaytransform_ )
		displaytransform_->transform( pos );

	    const float curdist = s2dd->calcDist( pos );
	    if ( curdist <= 0.5*onestepdist )
		curobjcoincides = true;
	    if ( curdist <= (mHideKnotTolerance+0.5)*onestepdist )
		showcols += rc.col();
	}
	res = res || curobjcoincides;

	for ( int idy=showcols.size()-1; curobjcoincides && idy>=0; idy-- )
	    fss.hideKnot( RowCol(sticknr,showcols[idy]), false, mSceneIdx );
    }

    return res;
}


bool FaultStickSetDisplay::coincidesWithPlane(
			Geometry::FaultStickSet& fss, int sticknr,
			TypeSet<Coord3>& intersectpoints )
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	  rowrg.snap(sticknr)!=sticknr )
	return res;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( RandomTrackDisplay*, rdtd, dataobj );
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	if ( !plane || !plane->isOn() )
	{
	    if ( !rdtd || !rdtd->isOn() )
		continue;
	}

	Coord3 nmpos = fss.getKnot( RowCol(rc.row(), rc.col()) );
	if ( displaytransform_ )
	    displaytransform_->transform( nmpos );

	const Coord3 vec1 = fss.getEditPlaneNormal(sticknr).normalize();
	const Coord3 vec2 = plane
			    ? plane->getNormal(Coord3()).normalize()
			    : rdtd->getNormal(nmpos).normalize();

	const bool coincidemode = fabs(vec1.dot(vec2)) > 0.5;
	const Coord3 planenormal = plane
				   ? plane->getNormal(Coord3::udf())
				   : rdtd->getNormal(nmpos);

	const double onestepdist = Coord3(1,1,mZScale()).dot(
	    s3dgeom_->oneStepTranslation(planenormal) );

	float prevdist = -1;
	Coord3 prevpos;

	bool curobjcoincides = false;
	TypeSet<int> showcols;
	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
							rc.col()+=colrg.step_ )
	{
	    Coord3 curpos = fss.getKnot(rc);

	    if ( displaytransform_ )
		displaytransform_->transform( curpos );

	    const float curdist = plane
				  ? plane->calcDist(curpos)
				  : rdtd->calcDist(curpos);
	    if ( curdist <= 0.5*onestepdist )
	    {
		intersectpoints += curpos;
		if ( coincidemode )
		    curobjcoincides = true;
	    }
            else if ( rc.col() != colrg.start_ )
	    {
		const float frac = prevdist / (prevdist+curdist);
		Coord3 interpos = (1-frac)*prevpos + frac*curpos;

		const float dist = plane
				   ? plane->calcDist( interpos )
				   : rdtd->calcDist( interpos );

		if ( dist <= 0.5*onestepdist )
		{
		    if ( prevdist <= 0.5*onestepdist )
			intersectpoints.removeSingle(intersectpoints.size()-1);

		    intersectpoints += interpos;
		    if ( coincidemode )
			curobjcoincides = true;
		}
	    }

	    if ( coincidemode && curdist<=(mHideKnotTolerance+0.5)*onestepdist )
		showcols += rc.col();

	    prevdist = curdist;
	    prevpos = curpos;
	}
	res = res || curobjcoincides;

	for ( int idy=showcols.size()-1; curobjcoincides && idy>=0; idy-- )
	    fss.hideKnot( RowCol(sticknr,showcols[idy]), false, mSceneIdx );
    }
    return res;
}


void FaultStickSetDisplay::displayOnlyAtSectionsUpdate()
{
    if ( !fault_ || !fsseditor_ || !viseditor_ )
	return;

    NotifyStopper ns( fsseditor_->editpositionchange );
    deepErase( stickintersectpoints_ );

    const EM::PosID curdragger = viseditor_->getActiveDragger();

    EM::FaultStickSet* emfss = emFaultStickSet();
    if ( !emfss )
	return;

    for ( int sidx=0; sidx<fault_->nrSections(); sidx++ )
    {
	mDynamicCastGet( Geometry::FaultStickSet*, fss,
			 emfss->geometryElement())
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row()=rowrg.start_; rc.row()<=rowrg.stop_;
	      rc.row()+=rowrg.step_ )
	{
	    const StepInterval<int> colrg = fss->colRange();
            for ( rc.col()=colrg.start_; rc.col()<=colrg.stop_;
                  rc.col()+=colrg.step_ )
	    {
		fss->hideKnot( rc, displayonlyatsections_, mSceneIdx );
		if ( curdragger==EM::PosID(emfss->id(),rc) )
		    fss->hideKnot( rc, false, mSceneIdx );
	    }

	    TypeSet<Coord3> intersectpoints;
	    fss->hideStick( rc.row(), displayonlyatsections_, mSceneIdx );
	    if ( !displayonlyatsections_ )
		continue;

	    if ( emfss->geometry().pickedOn2DLine(rc.row()) )
	    {
		const Pos::GeomID geomid =
				emfss->geometry().pickedGeomID(rc.row());
		if ( coincidesWith2DLine(*fss,rc.row(),geomid) )
		{
		    fss->hideStick( rc.row(), false, mSceneIdx );
		    continue;
		}
	    }

	    if ( coincidesWithPlane(*fss, rc.row(), intersectpoints) )
	    {
		if ( emfss->geometry().pickedOnPlane(rc.row()) )
		{
		    fss->hideStick( rc.row(), false, mSceneIdx );
		    continue;
		}
	    }

	    for (  int idx=0; idx<intersectpoints.size(); idx++ )
	    {
		auto* sip = new StickIntersectPoint();
		sip->sticknr_ = rc.row();
		sip->pos_ = intersectpoints[idx];
		if ( displaytransform_ )
		    displaytransform_->transformBack( sip->pos_ );

		stickintersectpoints_ += sip;
	    }
	}
    }
}


void FaultStickSetDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    if ( displayonlyatsections_ == yn )
	return;

    displayonlyatsections_ = yn;
    updateAll();
    displaymodechange.trigger();
}


bool FaultStickSetDisplay::displayedOnlyAtSections() const
{
    return displayonlyatsections_;
}


void FaultStickSetDisplay::setStickSelectMode( bool yn )
{
    if ( yn==stickselectmode_ )
	return;

    stickselectmode_ = yn;
    ctrldown_ = false;

    setActiveStick( EM::PosID::udf() );
    updateManipulator();
    updateEditPids();
    updateKnotMarkers();

    if ( scene_ && scene_->getPolySelection() )
    {
	if ( yn )
	    mAttachCBIfNotAttached(
		scene_->getPolySelection()->polygonFinished(),
		FaultStickSetDisplay::polygonFinishedCB );
	else
	    mDetachCB( scene_->getPolySelection()->polygonFinished(),
		       FaultStickSetDisplay::polygonFinishedCB );
    }

}


void FaultStickSetDisplay::turnOnSelectionMode( bool yn )
{
   setStickSelectMode( yn );
}


void FaultStickSetDisplay::polygonFinishedCB( CallBacker* cb )
{
    if ( !stickselectmode_ || !fault_ || !scene_ || !isOn() || !isSelected() )
	return;

    if ( getCurScene().ptr() != scene_ )
	setCurScene( scene_ );

    polygonSelectionCB();
}


bool FaultStickSetDisplay::isInStickSelectMode() const
{
    return stickselectmode_;
}


void FaultStickSetDisplay::updateKnotMarkers()
{
    if ( !fault_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    if ( getCurScene().ptr() != scene_ )
	setCurScene( scene_ );

    updateStickMarkerSet();
}


void FaultStickSetDisplay::updateAll()
{
    displayOnlyAtSectionsUpdate();
    updateSticks();
    updateEditPids();
    updateKnotMarkers();
}


void FaultStickSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
					    Coord3& pos, BufferString& val,
					    uiString& info ) const
{
    StickSetDisplay::getMousePosInfo( eventinfo, pos, val, info );
}


void FaultStickSetDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );
    par.set( sKeyEarthModelID(), getMultiID() );
    par.setYN( sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    par.set( sKey::Color(), (int) getColor().rgb() );
}


bool FaultStickSetDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar(par) ||
	 !SurveyObject::usePar(par) )
	return false;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMObjectID( emobject->id() );
    }

    par.getYN(  sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    OD::Color col;
    par.get( sKey::Color(), (int&) col.rgb() );
    setColor( col );

    return true;
}


void FaultStickSetDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( sticks_ )
	sticks_->setPixelDensity( dpi );

    if ( activestick_ )
	activestick_ ->setPixelDensity( dpi );

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->setPixelDensity( dpi );
}


void FaultStickSetDisplay::hideAllKnots( bool yn )
{
    if ( hideallknots_ != yn )
    {
	hideallknots_ = yn;
	updateAll();
	updateManipulator();
    }
}


const MarkerStyle3D* FaultStickSetDisplay::getPreferedMarkerStyle() const
{
    if ( fault_ )
	return &fault_->getPosAttrMarkerStyle( 0 );

    return nullptr;
}


void FaultStickSetDisplay::setPreferedMarkerStyle( const MarkerStyle3D& ms )
{
    // for stickset we do use fixed color for dragger, polygon, and selection.
    // So to guarantee this here we set a fixed color.

    MarkerStyle3D myms = ms;
    myms.color_ = OD::Color::Yellow();

    if ( viseditor_ )
	viseditor_->setMarkerStyle( myms );

    setStickMarkerStyle( myms );

    if ( fault_ )
	fault_->setPosAttrMarkerStyle( 0, myms );
}

} // namespace visSurvey
