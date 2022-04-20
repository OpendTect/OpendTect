/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/


#include "vislocationdisplay.h"

#include "color.h"
#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "selector.h"
#include "survgeom2d.h"
#include "zaxistransform.h"

#include "visevent.h"
#include "vismaterial.h"
#include "visseedpainter.h"
#include "vissower.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"
#include "vishorizondisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"


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
	return (float) mMIN( p1.distTo(p), p2.distTo(p) );

    const Coord3 proj = p1 + vec * factor;
    return (float) proj.distTo( p );
}


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , manip_( this )
{
    sower_ = new Sower( this );
    sower_->ref();
    addChild( sower_->osgNode() );

    painter_ = new SeedPainter;
    painter_->ref();
    addChild( painter_->osgNode() );

    setSetMgr( &Pick::Mgr() );
}


LocationDisplay::~LocationDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( nullptr );
    setSetMgr( nullptr );
    unRefPtr( transformation_ );

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    removeChild( sower_->osgNode() );
    unRefAndZeroPtr( sower_ );
    removeChild( painter_->osgNode() );
    unRefAndZeroPtr( painter_ );
}


void LocationDisplay::setSet( Pick::Set* ps )
{
    if ( !ps )
	return;

    if ( set_ )
    {
	if ( set_!=ps )
	{
	    pErrMsg("Cannot set set_ twice");
	}
	return;
    }

    set_ = ps;
    painter_->setSet( ps );
    setName( set_->name() );
    if ( picksetmgr_ )
    {
	const int setidx = picksetmgr_->indexOf( *set_ );
	storedmid_ =  picksetmgr_->id( setidx );
    }
    else
	storedmid_.setUdf();

    fullRedraw();

    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );
}


void LocationDisplay::setSetMgr( Pick::SetMgr* mgr )
{
    if ( picksetmgr_ )
	picksetmgr_->removeCBs( this );

    picksetmgr_ = mgr;
    painter_->setSetMgr( mgr );

    if ( picksetmgr_ )
    {
	mAttachCB( picksetmgr_->locationChanged , LocationDisplay::locChg );
	mAttachCB( picksetmgr_->bulkLocationChanged,
		   LocationDisplay::bulkLocChg );
	mAttachCB( picksetmgr_->setChanged, LocationDisplay::setChg );
	mAttachCB( picksetmgr_->setDispChanged, LocationDisplay::dispChg );
    }
}


RefMan<Pick::Set> LocationDisplay::getSet()
{
    return set_;
}


ConstRefMan<Pick::Set> LocationDisplay::getSet() const
{
    return set_;
}


void LocationDisplay::fullRedraw( CallBacker* )
{
    if ( !set_ )
	return;

    if ( datatransform_ && datatransform_->needsVolumeOfInterest() &&
	 !set_->isEmpty() )
    {
	TrcKeyZSampling cs( false );
	for ( int pidx=0; pidx<set_->size(); pidx++ )
	{
	    const Pick::Location& loc = set_->get( pidx );
	    BinID bid = SI().transform( loc.pos() );
	    const float zval = loc.z();
	    cs.hsamp_.include( bid );
	    cs.zsamp_.include( zval, false );
	}

	if ( cs.isDefined() )
	{
	    if ( voiidx_<0 )
		voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	    datatransform_->loadDataIfMissing( voiidx_ );
	}
    }

    getMaterial()->setColor( set_->disp_.color_ );
    invalidpicks_.erase();

    if ( set_->isEmpty() )
    {
	removeAll();
	return;
    }

    if ( set_->nrSets() > 1 )
    {
	redrawMultiSets();
	return;
    }

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	Pick::Location loc = set_->get( idx );
	if ( !transformPos(loc) )
	{
	    invalidpicks_ += idx;
	}
	else
	{
	    invalidpicks_ -= idx;
	}

	setPosition( idx, loc );
    }
}


void LocationDisplay::removeAll()
{
    if ( !set_ )
	return;
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


void LocationDisplay::setOnlyAtSectionsDisplay( bool yn )
{ showAll( !yn); }

bool LocationDisplay::displayedOnlyAtSections() const
{ return !allShown(); }


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( painter_ && painter_->isActive() )
	return;

    if ( !set_ || set_->isReadOnly() )
	return;

    if ( !isSelected() || !isOn() || isLocked() ) return;

    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.dragging )
	updateDragger();

    int eventid = -1;
    pickedsobjid_ = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
			visBase::DM().getObject( eventinfo.pickedobjids[idx] );
	if ( !dataobj || dataobj == this )
	    continue;

	if ( dataobj->isPickable() )
	    eventid = eventinfo.pickedobjids[idx];

	mDynamicCastGet(const SurveyObject*,so,dataobj);
	if ( so && so->allowsPicks() )
	    pickedsobjid_ = eventid;

	if ( pickedsobjid_ != -1 )
	    break;
    }

    if ( eventinfo.type == visBase::MouseClick && !eventinfo.pressed )
    {
	if ( !draggerNormal() )
	{
	    const Coord3 normal = getActivePlaneNormal( eventinfo );
	    setDraggerNormal( normal );
	}
	updateDragger();
    }

    const bool sowerenabled = set_->disp_.connect_ != Pick::Set::Disp::None;

    const bool allowdblclick = allowdoubleclicks_;
    if ( eventinfo.type == visBase::MouseDoubleClick && allowdblclick )
    {
	if ( set_ && set_->disp_.connect_!=Pick::Set::Disp::None )
	{
	    set_->disp_.connect_ = Pick::Set::Disp::Close;
	    dispChg( 0 );
	    return;
	}
    }

    if ( waitsfordirectionid_!=-1 )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    const Pick::Location& loc = set_->get( waitsfordirectionid_ );
	    Coord3 dir = newpos - loc.pos();
	    const float zscale = scene_ ? scene_->getZScale(): SI().zScale();
	    dir.z *= -zscale; //convert to right dir-domain
	    if ( dir.sqAbs()>=0 )
	    {
		set_->setDir( waitsfordirectionid_,
			      cartesian2Spherical(dir,true) );
		Pick::SetMgr::ChangeData cd(
			Pick::SetMgr::ChangeData::Changed,
			set_, waitsfordirectionid_ );
		picksetmgr_->reportChange( 0, cd );
	    }
	}

	eventcatcher_->setHandled();
    }
    else if ( waitsforpositionid_!=-1 ) // dragging
    {
	// when dragging it will receive multi times coords from visevent:
	// mouse move and mouse release. we need the last one and the begin
	// one for undo issue
	const Pick::Location& loc = set_->get( waitsforpositionid_ );
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    if ( eventinfo.type==visBase::MouseClick )
	    {
		if ( undoloccoord_.isDefined() )
		{
		    const ::Sphere dir = loc.dir();
		    Pick::Location undoloc( undoloccoord_, dir );
		    Pick::Location newloc( newpos, dir );
		    mDynamicCastGet(const Seis2DDisplay*,s2d,
				    getPickedSurveyObject())
		    if ( s2d )
		    {
			undoloc.setTrcKey( loc.trcKey() );
			newloc.setTrcKey( TrcKey(s2d->getGeomID(),
					s2d->getNearestTraceNr(newpos)) );
		    }

		    set_->moveWithUndo(
			waitsforpositionid_, undoloc, newloc );
		    Pick::Mgr().undo().setUserInteractionEnd(
			Pick::Mgr().undo().currentEventID() );
		    undomove_ = false;
		}
	    }
	    else
	    {
		if ( !undomove_ )
		{
		    undoloccoord_ = loc.pos();
		    undomove_ =  true;
		}

		set_->setPos( waitsforpositionid_, newpos );
	    }

	    Pick::SetMgr::ChangeData cd(
		    Pick::SetMgr::ChangeData::Changed,
		    set_, waitsforpositionid_ );
	    picksetmgr_->reportChange( 0, cd );
	}

	eventcatcher_->setHandled();
    }
    else if ( sowerenabled && sower_->accept(eventinfo) )
	return;

    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton( eventinfo.buttonstate_ ) )
	return;

    if ( eventid == -1 )
	return;

    if ( waitsforpositionid_!=-1 || waitsfordirectionid_!=-1 )
    {
	setPickable( true );
	waitsforpositionid_ = -1;
	waitsfordirectionid_ = -1;
	mousepressid_ = -1;
    }
    else if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    const int selfpickidx = clickedMarkerIndex( eventinfo );
	    if ( selfpickidx!=-1 )
	    {
		setPickable( false, false );
		waitsforpositionid_ = selfpickidx;
	    }
	    const int selfdirpickidx = isDirMarkerClick(eventinfo.pickedobjids);
	    if ( selfdirpickidx!=-1 )
	    {
		setPickable( false, false );
		waitsfordirectionid_ = selfpickidx;
	    }

	    //Only set handled if clicked on marker. Otherwise
	    //we may interfere with draggers.
	    if ( selfdirpickidx!=-1 || selfpickidx!=-1 )
	    {
		eventcatcher_->setHandled();
	    }
	    else
	    {
		const OD::Color& color = set_->disp_.color_;
		if ( sowerenabled && sower_->activate(color, eventinfo) )
		    return;
	    }
	}
    }
    else
    {
	if ( OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		const int removeidx = clickedMarkerIndex( eventinfo );
		if ( removeidx!=-1 )
		    removePick( removeidx );
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	          !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
		  !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		Coord3 newpos, normal;
		if ( getPickSurface(eventinfo,newpos,normal) )
		{
		    const Sphere dir = normal.isDefined()
			? cartesian2Spherical(
				Coord3(normal.y,-normal.x,normal.z), true)
			: Sphere( 1, 0, 0 );

		    if ( addPick( newpos, dir, true ) )
		    {
			if ( hasDirection() )
			{
			    setPickable( false, false );
			    waitsfordirectionid_ = set_->size()-1;
			}

			eventcatcher_->setHandled();
		    }
		}
	    }
	}
    }
}


bool LocationDisplay::getPickSurface( const visBase::EventInfo& evi,
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
	newpos.z = datatransform_->transformBack( newpos );
	if ( mIsUdf(newpos.z) )
	    return false;
    }

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


Coord3 LocationDisplay::convertCoords( const Coord3& crd, bool disptoworld )
{
    return disptoworld ? display2World( crd ) : world2Display( crd );
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
    if ( !datatransform_ ) return true;

    float newdepth = mUdf(float);
    if ( datatransform_->canTransformSurv(OD::Geom3D) )
	newdepth = datatransform_->transformTrc( loc.trcKey(), loc.z() );
    else if ( datatransform_->canTransformSurv(OD::Geom2D) )
    {
	if ( loc.trcKey().is2D() )
	    newdepth = datatransform_->transformTrc( loc.trcKey(), loc.z() );
	else // Pre v6.0 Pickset without TrcKey information
	{
	    BufferStringSet nms; TypeSet<Pos::GeomID> ids;
	    Survey::GM().getList( nms, ids, true );
	    TrcKey ntk = TrcKey::udf();
	    float ndist = mUdf(float);
	    for ( int idx=0; idx<ids.size(); idx++ )
	    {
		mDynamicCastGet(const Survey::Geometry2D*,geom2d,
				Survey::GM().getGeometry(ids[idx]))
		if ( !geom2d )
		    continue;

		float dist;
		const TrcKey tk = geom2d->nearestTrace( loc.pos(), &dist );
		if ( dist>ndist || dist>geom2d->averageTrcDist() )
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
    {
	pErrMsg("Direction not impl");
    }

    return true;
}


void LocationDisplay::locChg( CallBacker* cb )
{
    if ( !set_ || set_->isReadOnly() )
	return;

    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
    {
	pErrMsg("Wrong pointer passed");
	return;
    }
    else if ( cd->set_ != set_ )
	return;

    if ( cd->ev_==Pick::SetMgr::ChangeData::Added )
    {
	const int pickidx = cd->loc_;
	if ( !set_->validIdx(pickidx) )
	    return;

	Pick::Location loc = set_->get( pickidx );
	if ( !transformPos(loc) )
	{
	    invalidpicks_ += cd->loc_;
	}

	setPosition( cd->loc_,loc, true );
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::ToBeRemoved )
    {
	removePosition( cd->loc_ );
	invalidpicks_ -= cd->loc_;
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::Changed )
    {
	Pick::Location loc = set_->get( cd->loc_ );
	if ( !transformPos(loc) )
	{
	    if ( invalidpicks_.indexOf(cd->loc_)==-1 )
		invalidpicks_ += cd->loc_;
	}
	else
	{
	    invalidpicks_ -= cd->loc_;
	}

	setPosition( cd->loc_, loc );
    }
}


void LocationDisplay::bulkLocChg( CallBacker* cb )
{
    if ( !set_ || set_->isReadOnly() )
	return;

    mDynamicCastGet(Pick::SetMgr::BulkChangeData*,cd,cb)
    if ( !cd )
    {
	pErrMsg("Wrong pointer passed");
	return;
    }
    else if ( cd->set_ != set_ )
	return;

    if ( cd->ev_==Pick::SetMgr::BulkChangeData::Added )
    {
	const TypeSet<int>& pickidxs = cd->locs_;
	for ( int pidx=0; pidx<pickidxs.size(); pidx++ )
	{
	    const int pickidx = pickidxs[pidx];
	    if ( !set_->validIdx(pickidx) )
		return;

	    Pick::Location loc = set_->get( pickidx );
	    if ( !transformPos(loc) )
	    {
		invalidpicks_ += pickidx;
	    }

	    setPosition( pickidx, loc, true );
	}
    }
    else if ( cd->ev_==Pick::SetMgr::BulkChangeData::ToBeRemoved )
    {
	const TypeSet<int>& pickidxs = cd->locs_;
	for ( int pidx=pickidxs.size()-1; pidx>=0; pidx-- )
	{
	    const int pickidx = pickidxs[pidx];
	    removePosition( pickidx );
	    invalidpicks_ -= pickidx;
	}
    }
}


void LocationDisplay::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
    {
	pErrMsg("Wrong pointer passed");
	return;
    }
    else if ( ps != set_ )
	return;

    manip_.trigger();
    fullRedraw();
}


void LocationDisplay::dispChg( CallBacker* )
{
    if ( set_ )
	getMaterial()->setColor( set_->disp_.color_ );
}


void LocationDisplay::setColor( OD::Color nc )
{
    if ( set_ )
	set_->disp_.color_ = nc;
}


OD::Color LocationDisplay::getColor() const
{
    if ( set_ )
	return set_->disp_.color_;

    return OD::Color::DgbColor();
}


bool LocationDisplay::isPicking() const
{
    const bool isreadonly = set_ && set_->isReadOnly();
    return isSelected() && !isLocked() && !isreadonly;
}


bool LocationDisplay::isPainting() const
{
    return isPicking() && painter_ && painter_->isActive();
}

bool LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
			       bool notif )
{
    if ( selectionmodel_ ) return false;

    mDefineStaticLocalObject( TypeSet<Coord3>, sowinghistory, );

    int locidx = -1;
    bool insertpick = false;
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    {
	sower_->alternateSowingOrder( true );
	Coord3 displaypos = world2Display( pos );
	if ( sower_->mode() == Sower::FirstSowing )
	{
	    displaypos = sower_->pivotPos();
	    sowinghistory.erase();
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<set_->size(); idx++ )
	{
	    const int pidx = idx>0 ? idx-1 : set_->size()-1;
	    int nrmatches = sowinghistory.indexOf( set_->getPos(idx) ) >= 0;
	    nrmatches += sowinghistory.indexOf( set_->getPos(pidx) ) >= 0;
	    if ( nrmatches != sowinghistory.size() )
		continue;

	    const float dist = findDistance( world2Display(set_->getPos(pidx)),
					     world2Display(set_->getPos(idx)),
					     displaypos );
	    if ( mIsUdf(dist) )
		continue;

	    if ( mIsUdf(mindist) || dist<mindist )
	    {
		mindist = dist;
		locidx = idx;
	    }
	}
	insertpick = locidx >= 0;

	sowinghistory.insert( 0, pos );
	sowinghistory.removeSingle( 2 );
    }
    else
	sower_->alternateSowingOrder( false );

    Pick::Location newloc( pos, dir );
    mDynamicCastGet(const Seis2DDisplay*,s2d,getPickedSurveyObject())
    if ( s2d )
	newloc.setTrcKey( TrcKey(s2d->getGeomID(),
				 s2d->getNearestTraceNr(pos)) );

    if ( insertpick )
    {
	set_->insertWithUndo( locidx, newloc );
	Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );
    }
    else
    {
	set_->appendWithUndo( newloc );
	Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );
	locidx = set_->size()-1;
    }

    if ( notif && picksetmgr_ )
    {
	if ( picksetmgr_->indexOf(*set_)==-1 )
	    picksetmgr_->set( MultiID(), set_ );

	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				     set_, locidx );
	picksetmgr_->reportChange( 0, cd );
    }

    if ( !hasText() )
	return true;

    const Pick::Location& loc = set_->get( locidx );
    if ( !loc.hasText() || loc.text().isEmpty() )
    {
	removePick( locidx );
	return false;
    }

    return true;
}


void LocationDisplay::removePick( int removeidx, bool setundo )
{
    if ( set_->isReadOnly()  )
	return;

    if ( !picksetmgr_ )
	return;

    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				 set_, removeidx );
    set_->removeSingleWithUndo( removeidx );
    if ( setundo )
    {
	Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );
    }
    picksetmgr_->reportChange( 0, cd );
}


BufferString LocationDisplay::getManipulationString() const
{
    return BufferString( set_ && set_->isPolygon() ? "Polygon: " : "PointSet: ",
			 getName() );
}


void LocationDisplay::getObjectInfo( BufferString& info ) const
{ info = getManipulationString(); }


void LocationDisplay::getMousePosInfo( const visBase::EventInfo&,
				      Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ && invalidpicks_.isEmpty() ) return;

}


void LocationDisplay::setPosition( int idx, const Pick::Location& nl, bool add )
{
    if ( !set_ || !set_->validIdx(idx) )
	return;

    set_->set( idx, nl );
}


void LocationDisplay::removePosition( int idx )
{
    if ( !set_ || !set_->validIdx(idx) )
	return;

    set_->remove( idx );
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
    painter_->setDisplayTransformation( newtr );
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
    painter_->setEventCatcher( nevc );

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


int LocationDisplay::getPickIdx( visBase::DataObject* dataobj ) const
{
    return 0; // to be implemented
}


bool LocationDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
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


int LocationDisplay::clickedMarkerIndex(const visBase::EventInfo& evi) const
{
    return -1;
}


bool LocationDisplay::isMarkerClick(const visBase::EventInfo& evi) const
{
    return false;
}


int LocationDisplay::isDirMarkerClick(const TypeSet<int>&) const
{ return -1; }


void LocationDisplay::triggerDeSel()
{
    setPickable( true );
    waitsfordirectionid_ = -1;
    waitsforpositionid_ = -1;
    VisualObject::triggerDeSel();
}


const SurveyObject* LocationDisplay::getPickedSurveyObject() const
{
    const DataObject* pickedobj = visBase::DM().getObject( pickedsobjid_ );
    mDynamicCastGet(const SurveyObject*,so,pickedobj);
    return so;
}


bool LocationDisplay::removeSelections( TaskRunner* taskr )
{
    if ( set_->isReadOnly() )
	return false;

    bool changed = false;
    const Selector< Coord3>* selector = scene_ ? scene_->getSelector() : 0;
    if ( selector && selector->isOK() )
    {
	for ( int idx=set_->size()-1; idx>=0; idx-- )
	{
	    const Pick::Location& loc = set_->get( idx );
	    if ( selector->includes(loc.pos()) )
	    {
		removePick( idx, false );
		changed = true;
	    }
	}
    }

    if ( changed )
	Pick::Mgr().undo().setUserInteractionEnd(
	    Pick::Mgr().undo().currentEventID() );
    return changed;
}


void LocationDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    if ( !set_ ) return;

    if ( picksetmgr_ )
    {
	const int setidx = picksetmgr_->indexOf( *set_ );
	par.set( sKeyID(), setidx>=0 ? picksetmgr_->get(*set_) : "" );
	par.set( sKeyMgrName(), picksetmgr_->name() );
    }

    par.setYN( sKeyShowAll(), showall_ );
    par.set( sKeyMarkerType(), set_->disp_.markertype_ );
    par.set( sKeyMarkerSize(), set_->disp_.pixsize_ );

}


bool LocationDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	 return false;

    int markertype = 0;
    int pixsize = 3;
    par.get( sKeyMarkerType(), markertype );
    par.get( sKeyMarkerSize(), pixsize );

    bool shwallpicks = true;
    par.getYN( sKeyShowAll(), shwallpicks );
    showAll( shwallpicks );

    BufferString setmgr;
    if ( par.get(sKeyMgrName(),setmgr) )
	setSetMgr( &Pick::SetMgr::getMgr(setmgr.buf()) );

    if ( !par.get(sKeyID(),storedmid_) )
	return false;

    const int setidx = picksetmgr_ ? picksetmgr_->indexOf( storedmid_ ) : -1;
    if ( setidx==-1 )
    {
	RefMan<Pick::Set> newps = new Pick::Set;
	BufferString bs;
	PtrMan<IOObj> ioobj = IOM().get( storedmid_ );
	if ( ioobj )
	    PickSetTranslator::retrieve( *newps, ioobj, true, bs );

	if ( !newps->name() || !*newps->name() )
	    newps->setName( getName() );

	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;

	if ( picksetmgr_ )
	    picksetmgr_->set( storedmid_, newps );

	setSet( newps );
    }
    else
	setSet( picksetmgr_->get(storedmid_) );

    return true;
}


const Coord3 LocationDisplay::getActivePlaneNormal(
    const visBase::EventInfo& eventinfo ) const
{
    Coord3 normal = Coord3::udf();
    for ( int idx = 0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( !dataobj ) continue;
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	mDynamicCastGet( RandomTrackDisplay*, sdtd, dataobj );
	if ( plane && plane->isOn() )
	{
	    normal = plane->getNormal(Coord3::udf()).normalize();
	    break;
	}
	if ( sdtd && sdtd->isOn() )
	{
	    normal = sdtd->getNormal(eventinfo.displaypickedpos).normalize();
	    break;
	}
    }

    return normal;
}

} // namespace visSurvey
