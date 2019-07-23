/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/


#include "vislocationdisplay.h"

#include "keystrs.h"
#include "picksetmanager.h"
#include "selector.h"
#include "survgeom2d.h"

#include "visevent.h"
#include "vismaterial.h"
#include "vissower.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"
#include "vishorizondisplay.h"
#include "visrandomtrackdisplay.h"
#include "zaxistransform.h"


namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyMgrName()	{ return "Location.Manager"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }

static const float cDistEps = 0.1f;

static float findDistance( Coord3 p1, Coord3 p2, Coord3 p )
{
    const Coord3 vec = p2 - p1;
    const Coord3 newvec = p - p1;
    const float prod = (float) vec.dot(newvec);
    const float sq = (float) vec.sqAbs();
    if ( mIsZero(sq,cDistEps) ) return mUdf(float);	// p1 and p2 coincide.

    const float factor = prod / sq;
    if ( factor<0 || factor>1 )		// projected point outside the segment.
    {
	const float d1 = p1.distTo<float>(p);
	const float d2 = p2.distTo<float>(p);
	return mMIN( d1, d2 );
    }

    const Coord3 proj = p1 + vec * factor;
    return proj.distTo<float>( p );
}


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_( true )
    , set_(new Pick::Set)
    , manip_( this )
    , datatransform_( 0 )
    , pickedsurvobjid_(-1)
    , voiidx_(-1)
    , selectionmodel_(false)
    , ctrldown_(false)
    , allowdoubleclick_(true)
{
    sower_ = new Sower( this );
    addChild( sower_->osgNode() );
    mAttachCB( set_->objectChanged(), LocationDisplay::setChgCB );
}


LocationDisplay::~LocationDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    if ( transformation_ )
	transformation_->unRef();

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    removeChild( sower_->osgNode() );
    delete sower_;
}


void LocationDisplay::setSet( Pick::Set* ps )
{
    if ( set_.ptr() == ps )
	return;
    else if ( !ps )
	ps = new Pick::Set;

    if ( replaceMonitoredRef(set_,ps,this) )
    {
	setName( ps->name() );
	fullRedraw();
	if ( !showall_ && scene_ )
	    scene_->objectMoved( 0 );
    }
}


DBKey LocationDisplay::getDBKey() const
{
    return Pick::SetMGR().getID( *set_ ) ;
}


void LocationDisplay::fullRedraw( CallBacker* )
{
    invalidpicks_.erase();

    if ( set_->isEmpty() )
	{ removeAll(); return; }

    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	TrcKeyZSampling cs( false );
	Pick::SetIter psiter( *set_ );
	while ( psiter.next() )
	{
	    const Pick::Location& loc = psiter.get();
	    if ( loc.hasPos() )
	    {
		cs.hsamp_.include( loc.binID() );
		cs.zsamp_.include( (float)loc.pos().z_, false );
	    }
	}
	psiter.retire();

	if ( cs.isDefined() )
	{
	    if ( voiidx_<0 )
		voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	    datatransform_->loadDataIfMissing( voiidx_,
					       SilentTaskRunnerProvider() );
	}
    }

    getMaterial()->setColor( set_->dispColor() );

    // TODO make ID based.
    Pick::SetIter psiter( *set_ );
    int idx = 0;
    while ( psiter.next() )
    {
	Pick::Location loc = psiter.get();
	if ( !transformPos( loc ) )
	    invalidpicks_ += idx;
	else
	    invalidpicks_ -= idx;

	setPosition( idx, loc );
	idx++;
    }
}


void LocationDisplay::removeAll()
{
    // has to be done by subclass, nothing 2 do 4 me
}


void LocationDisplay::showAll( bool yn )
{
    showall_ = yn;
    if ( scene_ )
    {
	scene_->objectMoved(0);
	return;
    }
}


void LocationDisplay::allowDoubleClick( bool yn )
{
    allowdoubleclick_ = yn;
}


void LocationDisplay::setOnlyAtSectionsDisplay( bool yn )
{ showAll( !yn); }

bool LocationDisplay::displayedOnlyAtSections() const
{ return !allShown(); }


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !isOn() || isLocked() )
	return;

    mCBCapsuleUnpack( const EventInfo&, eventinfo, cb );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.dragging )
	updateDragger();

    if ( eventinfo.type == visBase::MouseClick && !eventinfo.pressed )
    {
	if ( !draggerNormal() )
	{
	    const Coord3 normal = getActivePlaneNormal( eventinfo );
	    setDraggerNormal( normal );
	}
	updateDragger();
    }

    const bool sowerenabled = set_->connection() != Pick::Set::Disp::None;

    if ( eventinfo.type==visBase::MouseDoubleClick && sowerenabled
						   && allowdoubleclick_ )
	{ set_->setConnection( Pick::Set::Disp::Close ); return; }

    if ( directionlocationid_.isValid() )
	handleDirectionEvent( eventinfo );
    else if ( movinglocationid_.isValid() ) // dragging
	handleDraggingEvent( eventinfo );
    else if ( sowerenabled && sower_->accept(eventinfo) )
	return;

    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton( eventinfo.buttonstate_ ) )
	return;

    const int eventid = getEventID( eventinfo );
    if ( eventid == -1 )
	return;

    if ( movinglocationid_.isValid() || directionlocationid_.isValid() )
    {
	setPickable( true ); mousepressid_ = -1;
	movinglocationid_.setInvalid(); directionlocationid_.setInvalid();
	return;
    }

    if ( eventinfo.pressed )
	handleMouseDown( eventinfo, eventid, sowerenabled );
    else
	handleMouseUp( eventinfo, eventid );
}


void LocationDisplay::handleDraggingEvent( const EventInfo& evinfo )
{
    Coord3 newpos, normal;
    if ( getPickSurface(evinfo,newpos,normal) )
    {
	Pick::Location pl = set_->get( movinglocationid_ );
	pl.setPos( newpos );
	const bool isstillmoving = evinfo.type != visBase::MouseClick;
	set_->set( movinglocationid_, pl, isstillmoving );
    }

    eventcatcher_->setHandled();
}


void LocationDisplay::handleDirectionEvent( const EventInfo& evinfo )
{
    Coord3 newpos, normal;
    if ( getPickSurface(evinfo,newpos,normal) )
    {
	Pick::Location pl( set_->get(directionlocationid_) );
	Coord3 dir = newpos - pl.pos();
	const float zscale = scene_ ? scene_->getZScale(): s3dgeom_->zScale();
	dir.z_ *= -zscale; //convert to right dir-domain
	if ( dir.sqAbs()>=0 )
	{
	    pl.setDir( cartesian2Spherical(dir,true) );
	    const bool isstillmoving = evinfo.type != visBase::MouseClick;
	    set_->set( directionlocationid_, pl, isstillmoving );
	}
    }

    eventcatcher_->setHandled();
}


int LocationDisplay::getEventID( const EventInfo& evinfo )
{
    int eventid = -1;
    pickedsurvobjid_ = -1;
    for ( int idx=0; idx<evinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
			visBase::DM().getObject( evinfo.pickedobjids[idx] );
	if ( !dataobj || dataobj == this )
	    continue;

	if ( dataobj->isPickable() )
	    eventid = evinfo.pickedobjids[idx];

	mDynamicCastGet(const SurveyObject*,so,dataobj);
	if ( so && so->allowsPicks() )
	    pickedsurvobjid_ = eventid;

	if ( pickedsurvobjid_ != -1 )
	    break;
    }
    return eventid;
}


void LocationDisplay::handleMouseDown( const EventInfo& evinfo,
					int eventid, bool sowerenabled )
{
    mousepressid_ = eventid;
    if ( !OD::ctrlKeyboardButton( evinfo.buttonstate_ ) &&
	 !OD::altKeyboardButton( evinfo.buttonstate_ ) &&
	 !OD::shiftKeyboardButton( evinfo.buttonstate_ ) )
    {
	const int locpickidx = clickedMarkerIndex( evinfo );
	if ( locpickidx >= 0 )
	{
	    setPickable( false, false );
	    movinglocationid_ = set_->locIDFor( locpickidx );
	}
	const int dirpickidx
			= isDirMarkerClick(evinfo.pickedobjids);
	if ( dirpickidx >= 0 )
	{
	    directionlocationid_ = set_->locIDFor( locpickidx );
	    setPickable( false, false );
	}

	//Only set handled if clicked on marker. Otherwise
	//we may interfere with draggers.
	if ( dirpickidx>=0 || locpickidx>=0 )
	    eventcatcher_->setHandled();
	else
	{
	    const Color& color = set_->dispColor();
	    if ( sowerenabled && sower_->activate(color,evinfo) )
		return;
	}
    }
}


void LocationDisplay::handleMouseUp( const EventInfo& evinfo,
					int eventid )
{
    if ( OD::ctrlKeyboardButton( evinfo.buttonstate_ ) &&
	 !OD::altKeyboardButton( evinfo.buttonstate_ ) &&
	 !OD::shiftKeyboardButton( evinfo.buttonstate_ ) )
    {
	if ( evinfo.pickedobjids.size() && eventid==mousepressid_ )
	{
	    const int locidx = clickedMarkerIndex( evinfo );
	    if ( locidx >= 0 )
		set_->remove( set_->locIDFor(locidx) );
	}

	eventcatcher_->setHandled();
    }
    else if ( !OD::ctrlKeyboardButton( evinfo.buttonstate_ ) &&
	      !OD::altKeyboardButton( evinfo.buttonstate_ ) &&
	      !OD::shiftKeyboardButton( evinfo.buttonstate_ ) )
    {
	if ( evinfo.pickedobjids.size() &&
	     eventid==mousepressid_ )
	{
	    Coord3 newpos, normal;
	    if ( getPickSurface(evinfo,newpos,normal) )
	    {
		const Sphere dir = normal.isDefined() && hasDirection()
		    ? cartesian2Spherical(
			    Coord3(normal.y_,-normal.x_,normal.z_), true)
		    : Sphere::nullSphere();

		LocID locid = addPick( newpos, dir );
		if ( locid.isValid() )
		{
		    if ( hasDirection() )
		    {
			setPickable( false, false );
			directionlocationid_ = locid;
		    }

		    eventcatcher_->setHandled();
		}
	    }
	}
    }
}


bool LocationDisplay::getPickSurface( const EventInfo& evi,
				      Coord3& newpos, Coord3& normal ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( pickedobj == this )
	    continue;

	if ( eventid==-1 && pickedobj->isPickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( so && so->allowsPicks() )
	{
	    validpicksurface = true;
	    normal = so->getNormal( evi.displaypickedpos );
	    if ( eventid!=-1 )
		break;
	}
    }

    if ( !validpicksurface )
	return false;

    newpos = evi.worldpickedpos;
    if ( datatransform_ )
    {
	newpos.z_ = datatransform_->transformBack( newpos );
	if ( mIsUdf(newpos.z_) )
	    return false;
    }

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


Coord3 LocationDisplay::display2World( const Coord3& pos ) const
{
    Coord3 res = pos;
    if ( scene_ )
	scene_->getTempZStretchTransform()->transformBack( res );
    if ( transformation_ )
	transformation_->transformBack( res );
    return res;
}


Coord3 LocationDisplay::world2Display( const Coord3& pos ) const
{
    Coord3 res;
    mVisTrans::transform( transformation_, pos, res );
    if ( scene_ )
	scene_->getTempZStretchTransform()->transform( res );
    return res;
}


bool LocationDisplay::transformPos( Pick::Location& loc ) const
{
    if ( !datatransform_ )
	return true;

    float newdepth = mUdf(float);
    if ( datatransform_->canTransformSurv(OD::VolBasedGeom) )
	newdepth = datatransform_->transformTrc( loc.trcKey(), loc.z() );
    else if ( datatransform_->canTransformSurv(OD::LineBasedGeom) )
    {
	if ( loc.trcKey().is2D() )
	    newdepth = datatransform_->transformTrc( loc.trcKey(), loc.z() );
	else // Pre v6.0 Pickset without TrcKey information
	{
	    GeomIDSet ids;
	    SurvGeom::list2D( ids );
	    TrcKey ntk;
	    double ndist = mUdf(double);
	    for ( int idx=0; idx<ids.size(); idx++ )
	    {
		const auto& geom2d = SurvGeom::get2D( ids[idx] );
		if ( geom2d.isEmpty() )
		    continue;

		double dist;
		const TrcKey tk( ids[idx],
			geom2d.nearestTracePosition(loc.pos().getXY(),&dist) );
		if ( dist>ndist || dist>geom2d.averageTrcDist() )
		    continue;

		ndist = dist;
		ntk = tk;
	    }

	    if ( ntk.isUdf() )
		return false;

	    newdepth = datatransform_->transformTrc( ntk, loc.z() );
	}
    }

    if ( mIsUdf(newdepth) )
	return false;

    loc.setZ( newdepth );

    if ( hasDirection() )
	{ pErrMsg("Direction not impl"); }

    return true;
}


void LocationDisplay::setChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );

    if ( chgdata.changeType() == Pick::Set::cDispChange() )
	dispChg();
    else
	locChg( chgdata );
}


void LocationDisplay::locChg( const Monitorable::ChangeData& chgdata )
{
    if ( chgdata.isEntireObject() )
	{ fullRedraw( 0 ); return; }

    mGetIDFromChgData( LocID, locid, chgdata );
    const int locidx = set_->idxFor( locid );
    if ( chgdata.changeType() == Pick::Set::cLocationInsert() )
    {
	Pick::Location loc = set_->get( locid );
	if ( !transformPos( loc ) )
	    invalidpicks_ += locidx;

	setPosition( locidx, loc, true );
    }
    else if ( chgdata.changeType() == Pick::Set::cLocationRemove() )
    {
	removePosition( locidx );
	invalidpicks_ -= locidx;
    }
    else if ( Pick::Set::isLocationChange( chgdata.changeType() ) )
    {
	Pick::Location loc = set_->get( locid );
	if ( transformPos( loc ) )
	    invalidpicks_ -= locidx;
	else
	{
	    if ( invalidpicks_.indexOf(locidx) < 0 )
		invalidpicks_ += locidx;
	}

	setPosition( locidx, loc );
    }
}


void LocationDisplay::dispChg()
{
    getMaterial()->setColor( set_->dispColor() );
}


void LocationDisplay::setColor( Color nc )
{
    set_->setDispColor( nc );
}


Color LocationDisplay::getColor() const
{
    return set_->dispColor();
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


LocationDisplay::LocID LocationDisplay::addPick( const Coord3& pos,
						 const Sphere& dir )
{
    if ( selectionmodel_ )
	return LocID::getInvalid();

    mDefineStaticLocalObject( TypeSet<Coord3>, sowinghistory, );

    LocID beforelocid = LocID::getInvalid();
    if ( set_->connection() != Pick::Set::Disp::Close )
	sower_->alternateSowingOrder( false );
    else
    {
	sower_->alternateSowingOrder( true );
	Coord3 displaypos = world2Display( pos );
	if ( sower_->mode() == Sower::FirstSowing )
	{
	    displaypos = sower_->pivotPos();
	    sowinghistory.erase();
	}

	float mindist = mUdf(float);
	MonitorLock ml( *set_ );
	const int setsz = set_->size();
	for ( int idx=0; idx<setsz; idx++ )
	{
	    int pidx = idx>0 ? idx-1 : setsz-1;

	    const Coord3 pcoord = set_->getByIndex( idx ).pos();
	    const Coord3 prevpcoord = set_->getByIndex( pidx ).pos();
	    int nrmatches = sowinghistory.indexOf( pcoord ) >= 0;
	    nrmatches += sowinghistory.indexOf( prevpcoord ) >= 0;
	    if ( nrmatches != sowinghistory.size() )
		continue;

	    const float dist = findDistance( world2Display(prevpcoord),
					     world2Display(pcoord),
					     displaypos );
	    if ( mIsUdf(dist) )
		continue;

	    if ( mIsUdf(mindist) || dist<mindist )
	    {
		mindist = dist;
		beforelocid = set_->locIDFor( idx );
	    }
	}
	ml.unlockNow();

	sowinghistory.insert( 0, pos );
	sowinghistory.removeSingle( 2 );
    }

    Pick::Location newloc( pos, dir );
    const SurveyObject* so = getPickedSurveyObject();
    if ( so )
	newloc.setGeomID( so->getGeomID() );

    LocID newlocid = LocID::getInvalid();
    if ( beforelocid.isValid() )
	newlocid = set_->insertBefore( beforelocid, newloc );
    else
	newlocid = set_->add( newloc );

    if ( hasText() && !set_->get(newlocid).hasText() )
	{ set_->remove( newlocid ); newlocid.setInvalid(); }

    return newlocid;
}


BufferString LocationDisplay::getManipulationString() const
{
    return BufferString( set_->isPolygon() ? "Polygon: " : "PointSet: ",
			 getName() );
}


void LocationDisplay::getObjectInfo( BufferString& info ) const
{
    info = getManipulationString();
}


void LocationDisplay::getMousePosInfo( const EventInfo& ei, IOPar& iop ) const
{
    return SurveyObject::getMousePosInfo( ei, iop );
}


void LocationDisplay::getMousePosInfo( const EventInfo&,
				      Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val.setEmpty();
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ && invalidpicks_.isEmpty() )
	return;
    // Ehhh? anything?
}


void LocationDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    sower_->setDisplayTransformation( newtr );
}


const mVisTrans* LocationDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void LocationDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
}


void LocationDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;
    sower_->setEventCatcher( nevc );

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


int LocationDisplay::getPickID(
			    visBase::DataObject* dataobj ) const
{
    return -1; // to be implemented
}


bool LocationDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tskr )
{
    if ( datatransform_==zat )
	return true;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    datatransform_ = zat;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		mCB( this, LocationDisplay, fullRedraw) );

	datatransform_->ref();
    }


    fullRedraw();
    showAll( datatransform_ && datatransform_->needsVolumeOfInterest() );
    return true;
}


const ZAxisTransform* LocationDisplay::getZAxisTransform() const
{
    return datatransform_;
}


int LocationDisplay::clickedMarkerIndex( const EventInfo& evi ) const
{ return -1; }


bool LocationDisplay::isMarkerClick( const EventInfo& evi ) const
{
    return false;
}


int LocationDisplay::isDirMarkerClick( const TypeSet<int>& ) const
{ return -1; }


void LocationDisplay::triggerDeSel()
{
    setPickable( true );
    directionlocationid_.setInvalid();
    movinglocationid_.setInvalid();
    VisualObject::triggerDeSel();
}


const SurveyObject* LocationDisplay::getPickedSurveyObject() const
{
    const DataObject* pickedobj = visBase::DM().getObject( pickedsurvobjid_ );
    mDynamicCastGet(const SurveyObject*,so,pickedobj);
    return so;
}


bool LocationDisplay::removeSelections( TaskRunner* taskr )
{
    bool changed = false;
    const Selector< Coord3>* selector = scene_ ? scene_->getSelector() : 0;
    if ( selector && selector->isOK() )
    {
	RefMan<Pick::Set> workps = new Pick::Set( *set_ );
	Pick::SetIter4Edit psiter( *workps );
	while ( psiter.next() )
	{
	    const Pick::Location& loc = psiter.get();
	    if ( selector->includes(loc.pos()) )
	    {
		psiter.removeCurrent();
		changed = true;
	    }
	}
	psiter.retire();
	if ( changed )
	    *set_ = *workps;
    }
    return changed;
}


void LocationDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    par.setYN( sKeyShowAll(), showall_ );
    par.set( sKeyID(), Pick::SetMGR().getID(*set_) );
    par.set( sKeyMgrName(), set_->category() );
    BufferString mkststr;
    set_->markerStyle().toString( mkststr );
    par.set( sKey::MarkerStyle(), mkststr );
}


bool LocationDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	 return false;

    bool shwallpicks = true;
    par.getYN( sKeyShowAll(), shwallpicks );
    showAll( shwallpicks );

    DBKey setid;
    if ( !par.get(sKeyID(),setid) )
	return false;
    RefMan<Pick::Set> newps = Pick::SetMGR().fetchForEdit( setid );
    if ( !newps )
	return false;

    const bool v6_or_earlier =
	( par.majorVersion()+par.minorVersion()*0.1 )>0 &&
	( par.majorVersion()+par.minorVersion()*0.1 )<=6;

    Pick::Set::Disp disp = newps->getDisp();
    if ( v6_or_earlier )
    {
	int markertype = 0;
	int pixsize = 3;
	par.get( sKeyMarkerType(), markertype );
	markertype ++;
	par.get( sKeyMarkerSize(), pixsize );
	disp.mkstyle_.type_=OD::MarkerStyle3D::Type(markertype);
	disp.mkstyle_.size_ = pixsize;
    }
    else
    {
	BufferString mkststr;
	par.get( sKey::MarkerStyle(), mkststr );
	disp.mkstyle_.fromString( mkststr );
    }
    newps->setDisp( disp );

    setSet( newps );
    return true;
}


const Coord3 LocationDisplay::getActivePlaneNormal(
					const EventInfo& evinfo ) const
{
    Coord3 normal = Coord3::udf();
    for ( int idx = 0; idx<evinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(evinfo.pickedobjids[idx]);
	if ( !dataobj ) continue;
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	mDynamicCastGet( RandomTrackDisplay*, sdtd, dataobj );
	mDynamicCastGet( HorizonDisplay*,hord,dataobj);
	if ( plane && plane->isOn() )
	{
	    normal = plane->getNormal(Coord3::udf()).normalize();
	    break;
	}
	if ( sdtd && sdtd->isOn() )
	{
	    normal = sdtd->getNormal(evinfo.displaypickedpos).normalize();
	    break;
	}
	if ( hord && hord->isOn() )
	{
	    normal = hord->getNormal(Coord3::udf()).normalize();
	    break;
	}
    }

    return normal;
}
} // namespace visSurvey
