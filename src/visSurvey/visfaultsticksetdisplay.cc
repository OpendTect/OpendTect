/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2008
________________________________________________________________________

-*/

#include "visfaultsticksetdisplay.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "faultstickseteditor.h"
#include "iopar.h"
#include "keystrs.h"
#include "monitor.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "uistrings.h"
#include "undo.h"
#include "viscoord.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vislines.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "viscoord.h"
#include "vissurvobj.h"
#include "visdrawstyle.h"
#include "limits.h"

namespace visSurvey
{

const char* FaultStickSetDisplay::sKeyEarthModelID()	{ return "EM ID"; }
const char* FaultStickSetDisplay::sKeyDisplayOnlyAtSections()
					{ return "Display only at sections"; }

#define mDefaultMarkerSize 3
#define mSceneIdx (scene_ ? scene_->fixedIdx() : -1)

FaultStickSetDisplay::FaultStickSetDisplay()
    : VisualObjectImpl(true)
    , StickSetDisplay( true )
    , colorchange(this)
    , displaymodechange(this)
    , viseditor_(0)
    , fsseditor_(0)
    , activesticknr_( mUdf(int) )
    , sticks_(visBase::Lines::create())
    , activestick_(visBase::Lines::create())
    , displayonlyatsections_(false)
    , makenewstick_( false )
    , activestickid_( EM::PosID::getInvalid() )
{
    sticks_->ref();
    stickdrawstyle_ = sticks_->addNodeState( new visBase::DrawStyle );
    stickdrawstyle_->ref();

    OD::LineStyle stickls( OD::LineStyle::Solid, 3 );
    stickdrawstyle_->setLineStyle( stickls );
    addChild( sticks_->osgNode() );

    activestick_->ref();
    activestickdrawstyle_ = activestick_->addNodeState(new visBase::DrawStyle);
    activestickdrawstyle_->ref();

    stickls.width_ += 2;
    activestickdrawstyle_->setLineStyle( stickls );
    addChild( activestick_->osgNode() );

    for ( int idx=0; idx<3; idx++ )
    {
	visBase::MarkerSet* markerset = visBase::MarkerSet::create();
	markerset->ref();
	addChild( markerset->osgNode() );
	markerset->setMarkersSingleColor( idx ? Color(0,255,0) :
						Color(255,0,255) );
	knotmarkersets_ += markerset;
    }

    activestick_->setPickable( false );
    activestick_->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				   false );
    sticks_->setPickable( false );
    sticks_->enableTraversal( visBase::cDraggerIntersecTraversalMask(), false );
    setCurScene( scene_ );
}


FaultStickSetDisplay::~FaultStickSetDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );
    showManipulator( false );

    if ( viseditor_ )
	viseditor_->unRef();
    if ( fsseditor_ )
	fsseditor_->unRef();

    if ( fault_ )
    {
	MPE::engine().removeEditor( fault_->id() );
	fault_->objectChanged().remove(
		mCB(this,FaultStickSetDisplay,emChangeCB) );

    }
    fsseditor_ = 0;

    sticks_->removeNodeState( stickdrawstyle_ );
    stickdrawstyle_->unRef();
    sticks_->unRef();

    activestick_->removeNodeState( activestickdrawstyle_ );
    activestickdrawstyle_->unRef();
    activestick_->unRef();
}


void FaultStickSetDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
				    mCB(this,FaultStickSetDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
				    mCB(this,FaultStickSetDisplay,mouseCB) );
    }

    if ( viseditor_ )
	viseditor_->setSceneEventCatcher( eventcatcher_ );
}


void FaultStickSetDisplay::setScene( Scene* scn )
{
    SurveyObject::setScene( scn );

    if ( fsseditor_ ) fsseditor_->setSceneIdx( mSceneIdx );
}


DBKey FaultStickSetDisplay::getEMObjectID() const
{ return fault_ ? fault_->id() : DBKey::getInvalid(); }


#define mSetStickIntersectPointColor( color ) \
     knotmarkersets_[2]->setMarkersSingleColor( color );

#define mErrRet(s) { errmsg_ = s; return false; }

bool FaultStickSetDisplay::setEMObjectID( const DBKey& emid )
{
    if ( fault_ )
    {
	fault_->objectChanged().remove(
				    mCB(this,FaultStickSetDisplay,emChangeCB) );
	fault_->unRef();
    }

    fault_ = 0;
    if ( fsseditor_ )
    {
	fsseditor_->setEditIDs( 0 );
	fsseditor_->unRef();
    }
    fsseditor_ = 0;
    if ( viseditor_ )
	viseditor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return false;

    fault_ = (EM::Fault*) emfss;
    fault_->objectChanged().notify( mCB(this,FaultStickSetDisplay,emChangeCB) );
    fault_->ref();

    if ( !emfss->name().isEmpty() )
    {
	setName( emfss->name() );
	sticks_->setName( emfss->name() );
    }

    hideallknots_ = !fault_->isEmpty();

    if ( !viseditor_ )
    {
	viseditor_ = MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	viseditor_->sower().setIfDragInvertMask();
	addChild( viseditor_->osgNode() );
	mAttachCB( viseditor_->draggingStarted,
		   FaultStickSetDisplay::draggingStartedCB );
    }
    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    fsseditor_ = fsseditor;
    if ( fsseditor_ )
    {
	fsseditor_->ref();
	fsseditor_->setSceneIdx( mSceneIdx );
	fsseditor_->setEditIDs( &editpids_ );
    }

    viseditor_->setEditor( fsseditor_ );
    mAttachCB( viseditor_->sower().sowingEnd,
	       FaultStickSetDisplay::sowingFinishedCB );
    getMaterial()->setColor( fault_->preferredColor() );

    mSetStickIntersectPointColor( fault_->preferredColor() );
    viseditor_->setMarkerSize( mDefaultMarkerSize );

    setMarkerStyle( fault_->preferredMarkerStyle3D() );
    updateSticks();
    updateKnotMarkers();
    updateManipulator();
    return true;
}


DBKey FaultStickSetDisplay::getDBKey() const
{
    return fault_ ? fault_->dbKey() : DBKey::getInvalid();
}


void FaultStickSetDisplay::setColor( Color nc )
{
    if ( fault_ )
	fault_->setPreferredColor( nc );
    else
	getMaterial()->setColor( nc );

    colorchange.trigger();
}


NotifierAccess* FaultStickSetDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultStickSetDisplay::getColor() const
{ return getMaterial()->getColor(); }


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


void FaultStickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );

    sticks_->setDisplayTransformation( nt );
    activestick_->setDisplayTransformation( nt );

    StickSetDisplay::setDisplayTransformation( nt );
}


const mVisTrans* FaultStickSetDisplay::getDisplayTransformation() const
{ return StickSetDisplay::getDisplayTransformation(); }


void FaultStickSetDisplay::updateEditPids()
{
    if ( !fault_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    editpids_.erase();

    RowCol rc;
    const StepInterval<int> rowrg = fault_->rowRange();
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	if ( fault_->isStickHidden(rc.row(),mSceneIdx) )
	    continue;

	const StepInterval<int> colrg = fault_->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
		rc.col()+=colrg.step )
	{
	    if ( !fault_->isKnotHidden(rc,mSceneIdx) )
		editpids_ += EM::PosID::getFromRowCol( rc );
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
{ addPolyLineCoordIdx( coordidxlist, -1 ); }


void FaultStickSetDisplay::updateSticks( bool activeonly )
{
    if ( !fault_ || !viseditor_ || !&viseditor_->sower() ||
	viseditor_->sower().moreToSow() )
	return;

    visBase::Lines* poly = activeonly ? activestick_ : sticks_;

    const unsigned int maxpos = fault_->totalSize();
    poly->removeAllPrimitiveSets();
    Geometry::IndexedPrimitiveSet* primitiveset =
		    Geometry::IndexedPrimitiveSet::create( maxpos>USHRT_MAX );
    poly->addPrimitiveSet( primitiveset );

    if ( poly->getCoordinates()->size() )
	poly->getCoordinates()->setEmpty();

    MonitorLock ml( *fault_ );
    RowCol rc;
    TypeSet<int> coordidxlist;
    const StepInterval<int> rowrg = fault_->rowRange();
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	if ( activeonly && rc.row()!=activesticknr_ )
	    continue;

	if ( fault_->isStickHidden(rc.row(),mSceneIdx) )
	    continue;

	Seis2DDisplay* s2dd = 0;
	if (fault_->pickedOn2DLine( rc.row()) )
	{
	    const Pos::GeomID geomid = fault_->pickedGeomID( rc.row() );
	    if ( geomid.isValid() )
		s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	}

	const StepInterval<int> colrg = fault_->colRange( rc.row() );

	if ( !colrg.width() )
	{
	    rc.col() = colrg.start;
	    if ( isSelected() || fault_->isKnotHidden(rc,mSceneIdx) )
		continue;

	    for ( int dim=0; dim<3; dim++ )
	    {
		const float step = dim==2 ? s3dgeom_->zRange().step
					    : s3dgeom_->inlDistance();

		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fault_->getKnot( rc );
		    pos[dim] += step * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    addPolyLineCoordIdx( coordidxlist, ci );
		}
		addPolyLineCoordBreak( coordidxlist );
	    }
	    continue;
	}

	for ( rc.col()=colrg.start; rc.col()<colrg.stop;
		rc.col()+=colrg.step )
	{
	    if ( fault_->isKnotHidden(rc,mSceneIdx) )
		continue;

	    RowCol nextrc( rc );
	    nextrc.col() += colrg.step;

	    if ( fault_->isKnotHidden(nextrc,mSceneIdx) )
	    {
		addPolyLineCoordBreak( coordidxlist );
		continue;
	    }

	    TypeSet<Coord3> coords;
	    coords += fault_->getKnot( rc );
	    coords += fault_->getKnot( nextrc );

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

    ml.unlockNow();


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


static float zdragoffset = 0;

#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getFixedZStretch()\
    : s3dgeom_->zScale() )\

#define mSetUserInteractionEnd() \
    if ( !viseditor_->sower().moreToSow() ) \
	EM::FSSMan().undo(fault_->id()).setUserInteractionEnd( \
			EM::FSSMan().undo(fault_->id()).currentEventID() );


void FaultStickSetDisplay::sowingFinishedCB( CallBacker* )
{
    makenewstick_ = true;
}


void FaultStickSetDisplay::mouseCB( CallBacker* cb )
{
    if ( stickselectmode_ )
	return stickSelectCB( cb );

    if ( !fault_ || !fsseditor_ || !viseditor_ || !viseditor_->isOn() ||
	 !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack( const visBase::EventInfo&,eventinfo,cb);

    fsseditor_->setSowingPivot( disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;

    const EM::PosID mousepid =
		    viseditor_->mouseClickDragger( eventinfo.pickedobjids );

    if ( eventinfo.buttonstate_ == OD::ControlButton )
    {
	 makenewstick_ =  false;
	 if ( !activestickid_.isInvalid() )
	     fsseditor_->setLastClicked( activestickid_ );
	 return;
    }

    if ( eventinfo.type == visBase::MouseDoubleClick ||
	 eventinfo.buttonstate_ == OD::ShiftButton )
    {
	makenewstick_ = true;
	return;
    }

    PlaneDataDisplay* plane = 0;
    Seis2DDisplay* s2dd = 0;
    RandomTrackDisplay* rdtd = 0;
    HorizonDisplay* hordisp = 0;
    const DBKey* pickeddbkey = 0;
    Pos::GeomID pickedgeomid;
    const char* pickednm = 0;
    PtrMan<Coord3> normal = 0;
    ConstPtrMan<DBKey> horid;
    BufferString horshiftname;
    Coord3 pos;

    if ( !mousepid.isInvalid() )
    {
	const int sticknr = mousepid.getRowCol().row();
	pos = fault_->getPos( mousepid );
	pickeddbkey = fault_->pickedDBKey( sticknr );
	pickednm = fault_->pickedName( sticknr );
	pickedgeomid = fault_->pickedGeomID( sticknr );
	zdragoffset = 0;
    }
    else
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	    mDynamicCast(Seis2DDisplay*,s2dd,dataobj);
	    if ( s2dd )
	    {
		pickedgeomid = s2dd->getGeomID();
		break;
	    }
	    mDynamicCast(HorizonDisplay*,hordisp,dataobj);
	    if ( hordisp )
	    {
		horid = new DBKey( hordisp->getDBKey() );
		pickeddbkey = horid;
		horshiftname = hordisp->getTranslation().z_ *
		    scene_->zDomainInfo().userFactor();
		pickednm = horshiftname.buf();
		break;
	    }
	    mDynamicCast(PlaneDataDisplay*,plane,dataobj);
	    if ( plane )
	    {
		normal = new Coord3( plane->getNormal(Coord3::udf()) );
		break;
	    }
	    mDynamicCast(RandomTrackDisplay*,rdtd,dataobj);
	    if ( rdtd )
	    {
		normal =
		    new Coord3( rdtd->getNormal(eventinfo.displaypickedpos) );
		break;
	    }
	    mDynamicCastGet(FaultStickSetDisplay*,fssd,dataobj);
	    if ( fssd )
		return;
	}

	if ( !s2dd && !plane && !hordisp && !rdtd )
	{
	    setActiveStick( EM::PosID::getInvalid() );
	    return;
	}

	pos = disp2world( eventinfo.displaypickedpos );
    }

    EM::PosID insertpid;
    fsseditor_->setZScale( mZScale() );
    fsseditor_->getInteractionInfo( insertpid, pickeddbkey, pickednm,
				    pickedgeomid, pos, normal);

    if ( mousepid.isInvalid() && !viseditor_->isDragging() )
    {
	EM::PosID npid = fsseditor_->getNearestStick( pos,pickedgeomid,normal );
	if ( !npid.isInvalid() )
	{
	   setActiveStick( npid );
	   activestickid_ = npid;
	}
    }

    if ( locked_ || !pos.isDefined() ||
	 eventinfo.type!=visBase::MouseClick || viseditor_->isDragging() )
	return;

    if ( !mousepid.isInvalid() )
    {
	fsseditor_->setLastClicked( mousepid );
	setActiveStick( mousepid );
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;


    if ( !mousepid.isInvalid() && OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
    {
	// Remove knot/stick
	eventcatcher_->setHandled();
	if ( eventinfo.pressed )
	    return;

	editpids_.erase();
	const int rmnr = mousepid.getRowCol().row();
	if ( fault_->nrKnots(rmnr) == 1 )
	    fault_->removeStick( rmnr, true );
	else
	    fault_->removeKnot( mousepid, true );

	mSetUserInteractionEnd();
	updateEditPids();
	return;
    }

    if ( !mousepid.isInvalid() || OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	return;

    if ( viseditor_->sower().activate(fault_->preferredColor(), eventinfo) )
	return;

    if ( eventinfo.pressed )
	return;

    if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) ||
	 insertpid.isInvalid() || makenewstick_ )
    {
	// Add stick
	const Coord3 editnormal(
		    plane || rdtd ? *normal :
		    hordisp ? Coord3(0,0,1) :
		    Coord3(s2dd->getNormal(s2dd->getNearestTraceNr(pos)),0) );

	const int insertsticknr = fault_->isEmpty() ? 0
						    : fault_->rowRange().stop+1;

	editpids_.erase();

	if ( !pickedgeomid.isValid() )
	    fault_->insertStick( insertsticknr, 0, pos, editnormal,
			      pickeddbkey, pickednm, true );
	else
	    fault_->insertStick( insertsticknr, 0, pos, editnormal,
			      pickedgeomid, true );

	const EM::PosID subid = EM::PosID::getFromRowCol( insertsticknr, 0 );
	fsseditor_->setLastClicked( subid );
	setActiveStick( subid );
	mSetUserInteractionEnd();
	updateEditPids();
	makenewstick_ = false;
    }
    else
    {
	// Add knot
	editpids_.erase();
	fault_->insertKnot( insertpid, pos, true );
	fsseditor_->setLastClicked( insertpid );
	mSetUserInteractionEnd();
	updateEditPids();
    }

    eventcatcher_->setHandled();
}


void FaultStickSetDisplay::draggingStartedCB( CallBacker* )
{
    fsseditor_->setLastClicked( viseditor_->getActiveDragger() );
    setActiveStick( viseditor_->getActiveDragger() );
}


void FaultStickSetDisplay::stickSelectCB( CallBacker* cb )
{
    if ( !fault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;
    if ( getCurScene() != scene_ )
	setCurScene( scene_ );
    stickSelectionCB( cb, get3DSurvGeom() );
}


void FaultStickSetDisplay::setActiveStick( const EM::PosID& pid )
{
    if ( !viseditor_ )
	return;

    const bool allowactivestick = viseditor_->isOn() && !pid.isInvalid();
    const int sticknr = allowactivestick ? pid.getRowCol().row() : mUdf(int);

    if ( activesticknr_ != sticknr )
    {
	activesticknr_ = sticknr;
	updateSticks( true );
    }
}


void FaultStickSetDisplay::emChangeCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, cbdata );
    RefMan<EM::ChangeAuxData> cbaux =	cbdata.auxDataAs<EM::ChangeAuxData>();
    if ( cbdata.changeType()==EM::Object::cPrefColorChange() )
    {
	getMaterial()->setColor( fault_->preferredColor() );
	mSetStickIntersectPointColor( fault_->preferredColor() );
    }
    if ( cbdata.changeType()==EM::Object::cPositionChange() && cbaux )
    {
	RowCol rc = cbaux->pid0.getRowCol();

	const DBKey* mid = fault_->pickedDBKey( rc.row() );
	const Pos::GeomID geomid = fault_->pickedGeomID( rc.row() );
	if ( fault_->pickedOnPlane(rc.row()) )
	{
	    const char* nm =fault_->pickedName( rc.row() );
	    const Coord3 dragpos = fault_->getPos( cbaux->pid0 );
	    Coord3 pos = dragpos;

	    Seis2DDisplay* s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	    if ( s2dd )
		pos = s2dd->getNearestSubPos( pos, true );

	    HorizonDisplay* hordisp = mid ?
			HorizonDisplay::getHorizonDisplay( *mid ) : 0;
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
		    {
			pos.z_ +=
			    toFloat(nm)/scene_->zDomainInfo().userFactor();
		    }

		    zdragoffset = (float) ( pos.z_ - dragpos.z_ );
		}
	    }

	    /*CallBack cb = mCB(this,FaultStickSetDisplay,emChangeCB);
	    emfss->objectChanged().remove( cb );
	    emfss->setPos( cbaux->pid0, pos, false );
	    emfss->objectChanged().notify( cb );*/
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

    if ( scene_ )
	scene_->blockMouseSelection( show );
}


bool  FaultStickSetDisplay::isManipulatorShown() const
{
    return showmanipulator_;
}


bool FaultStickSetDisplay::removeSelections( TaskRunner* taskr )
{
    if ( !fault_ )
	return false;

    fault_->removeSelectedSticks( true );
    fault_->setChangedFlag();
    return true;
}


void FaultStickSetDisplay::otherObjectsMoved(
				const ObjectSet<const SurveyObject>& objs,
				int whichobj )
{
    if ( !displayonlyatsections_ )
	return;

    updateAll();
}


#define mHideKnotTolerance 2.0

bool FaultStickSetDisplay::coincidesWith2DLine( int sticknr, Pos::GeomID geomid,
						TypeSet<RowCol>& knots )
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fault_->rowRange();
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
	MonitorLock ml( *fault_ );
	const StepInterval<int> colrg = fault_->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 pos = fault_->getKnot( rc );
	    if ( displaytransform_ )
		displaytransform_->transform( pos );

	    const float curdist = s2dd->calcDist( pos );
	    if ( curdist <= 0.5*onestepdist )
		curobjcoincides = true;
	    if ( curdist <= (mHideKnotTolerance+0.5)*onestepdist )
		knots += RowCol( sticknr, rc.col() );
	}

	res = res || curobjcoincides;
    }

    return res;
}


bool FaultStickSetDisplay::coincidesWithPlane( int sticknr,
					      TypeSet<Coord3>& intersectpoints,
					      TypeSet<RowCol>& knots )
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fault_->rowRange();
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
	Coord3 nmpos = fault_->getKnot( RowCol(rc.row(), rc.col()) );
	if ( displaytransform_ )
	    displaytransform_->transform( nmpos );

	const Coord3 vec1 = fault_->getEditPlaneNormal(sticknr).normalize();
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
	MonitorLock ml( *fault_ );
	const StepInterval<int> colrg = fault_->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 curpos = fault_->getKnot(rc);

	    if ( displaytransform_ )
		displaytransform_->transform( curpos );

	    const float curdist = plane
				  ? plane->calcDist( curpos )
				  : rdtd->calcDist( curpos );
	    if ( curdist <= 0.5*onestepdist )
	    {
		intersectpoints += curpos;
		if ( coincidemode )
		    curobjcoincides = true;
	    }
	    else if ( rc.col() != colrg.start )
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
		knots += RowCol( sticknr, rc.col() );

	    prevdist = curdist;
	    prevpos = curpos;
	}
	res = res || curobjcoincides;
    }
    return res;
}


void FaultStickSetDisplay::displayOnlyAtSectionsUpdate()
{
    if ( !fault_ || !fsseditor_ )
	return;

    NotifyStopper ns( fsseditor_->editpositionchange );
    deepErase( stickintersectpoints_ );

    fault_->hideAllSticks( displayonlyatsections_, mSceneIdx );
    fault_->hideAllKnots( displayonlyatsections_, mSceneIdx );

    if ( !displayonlyatsections_ )
	return;

    const EM::PosID curdragger = viseditor_->getActiveDragger();

    RowCol rc;
    MonitorLock ml( *fault_ );
    const StepInterval<int> rowrg = fault_->rowRange();
    TypeSet<RowCol> unhiddenknots;
    TypeSet<int> unhiddensticks;

    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	const StepInterval<int> colrg = fault_->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    if ( curdragger==EM::PosID::getFromRowCol(rc) )
		unhiddenknots += rc;
	}

	if ( fault_->pickedOn2DLine(rc.row()) )
	{
	    const Pos::GeomID geomid = fault_->pickedGeomID(rc.row());
	    if ( coincidesWith2DLine(rc.row(),geomid,unhiddenknots) )
	    {
		unhiddensticks += rc.row();
		continue;
	    }
	}

        TypeSet<Coord3> intersectpoints;
	if ( coincidesWithPlane(rc.row(),intersectpoints,unhiddenknots) )
	{
	    if ( fault_->pickedOnPlane(rc.row()) )
	    {
		unhiddensticks += rc.row();
		continue;
	    }
	}

	for (  int idx=0; idx<intersectpoints.size(); idx++ )
	{
	    StickIntersectPoint* sip = new StickIntersectPoint();
	    sip->sid_ = 0;
	    sip->sticknr_ = rc.row();
	    sip->pos_ = intersectpoints[idx];
	    if ( displaytransform_ )
		displaytransform_->transformBack( sip->pos_ );

	    stickintersectpoints_ += sip;
	}
    }

    ml.unlockNow();
    fault_->hideKnots( unhiddenknots, false, mSceneIdx );
    fault_->hideSticks( unhiddensticks, false, mSceneIdx );
}


void FaultStickSetDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    if ( displayonlyatsections_ == yn ) return;
    displayonlyatsections_ = yn;

    updateAll();
    displaymodechange.trigger();
}


bool FaultStickSetDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


void FaultStickSetDisplay::setStickSelectMode( bool yn )
{
    if ( yn==stickselectmode_ )
	return;

    stickselectmode_ = yn;
    ctrldown_ = false;

    setActiveStick( EM::PosID::getInvalid() );
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
	    mDetachCB(
	    scene_->getPolySelection()->polygonFinished(),
	    FaultStickSetDisplay::polygonFinishedCB );
    }

}


void FaultStickSetDisplay::turnOnSelectionMode( bool yn )
{  setStickSelectMode( yn ); }


void FaultStickSetDisplay::polygonFinishedCB( CallBacker* cb )
{
    if ( !stickselectmode_ || !fault_ || !scene_ || !isOn() || !isSelected() )
	return;
    if ( getCurScene() != scene_ )
	setCurScene( scene_ );
    polygonSelectionCB();
}


bool FaultStickSetDisplay::isInStickSelectMode() const
{ return stickselectmode_; }


void FaultStickSetDisplay::updateKnotMarkers()
{
    if ( !fault_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    if ( getCurScene() != scene_ )
	setCurScene(scene_);
    updateStickMarkerSet();
}


void FaultStickSetDisplay::updateAll()
{
    displayOnlyAtSectionsUpdate();
    updateSticks();
    updateEditPids();
    updateKnotMarkers();
}


void FaultStickSetDisplay::getMousePosInfo(const visBase::EventInfo& eventinfo,
    Coord3& pos,BufferString& val,
    BufferString& info) const
{
    StickSetDisplay::getMousePosInfo(eventinfo,pos,val,info);
}


void FaultStickSetDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    par.set( sKeyEarthModelID(), getDBKey() );
    par.setYN( sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    par.set( sKey::Color(), (int) getColor().rgb() );
}


bool FaultStickSetDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

    DBKey newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	SilentTaskRunnerProvider trprov;
	ConstRefMan<EM::Object> emobject = EM::FSSMan().fetch( newmid, trprov );
	if ( emobject ) setEMObjectID( emobject->id() );
    }

    par.getYN(  sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    Color col;
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

    for ( int idx =0; idx<knotmarkersets_.size(); idx++ )
        knotmarkersets_[idx]->setPixelDensity( dpi );
}


bool FaultStickSetDisplay::areAllKnotsHidden() const
{ return hideallknots_; }


void FaultStickSetDisplay::hideAllKnots( bool yn )
{
    if ( hideallknots_ != yn )
    {
	hideallknots_ = yn;
	updateAll();
	updateManipulator();
    }
}


const OD::MarkerStyle3D* FaultStickSetDisplay::markerStyle() const
{
    return 0;
}


void FaultStickSetDisplay::setMarkerStyle( const OD::MarkerStyle3D& mkstyle )
{
    // for stickset we do use fixed color for dragger, polygon, and selection.
    // So to guarantee this here we set a fixed color.

    OD::MarkerStyle3D sstmkstyle = mkstyle;
    sstmkstyle.color_ = Color::Yellow();

    viseditor_->setMarkerStyle( sstmkstyle );
    setStickMarkerStyle( sstmkstyle );

    if ( fault_ )
	fault_->setPreferredMarkerStyle3D( sstmkstyle );
}


} // namespace visSurvey
