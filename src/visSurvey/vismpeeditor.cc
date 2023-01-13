/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismpeeditor.h"

#include "color.h"
#include "emeditor.h"
#include "emsurfacegeometry.h"
#include "math2.h"

#include "visdragger.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vissower.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::MPEEditor )

namespace visSurvey
{

MPEEditor::MPEEditor()
    : visBase::VisualObjectImpl(false)
    , nodeRightClick(this)
    , emeditor_(nullptr)
    , eventcatcher_(nullptr)
    , transformation_(nullptr)
    , markersize_(3)
    , activedragger_(EM::PosID::udf())
    , activenodematerial_(nullptr)
    , nodematerial_(nullptr)
    , isdragging_(false)
    , draggerinmotion_(false)
    , draggingStarted(this)
    , patchmarkers_(nullptr)
    , patchline_(nullptr)
    , patch_(nullptr)
{
    nodematerial_ = new visBase::Material;
    nodematerial_->ref();
    nodematerial_->setColor( OD::Color(255,255,0) );

    activenodematerial_ = new visBase::Material;
    activenodematerial_->ref();
    activenodematerial_->setColor( OD::Color(255,0,0) );

    sower_ = new Sower( this );
    addChild( sower_->osgNode() );
    markerstyle_ = MarkerStyle3D::Cube;
    markerstyle_.size_ = (int)markersize_;
}


MPEEditor::~MPEEditor()
{
    setEditor( (MPE::ObjectEditor*) 0 );
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );

    while ( draggers_.size() )
	removeDragger( 0 );

    if ( activenodematerial_ ) activenodematerial_->unRef();
    if ( nodematerial_ ) nodematerial_->unRef();

    removeChild( sower_->osgNode() );
    delete sower_;

    unRefAndNullPtr( patchmarkers_ );
    unRefAndNullPtr( patchline_ );
}


void MPEEditor::setupPatchDisplay()
{
    OD::Color lineclr = OD::Color::Green() ;
    OD::Color mkclr = OD::Color::White();
    if ( emeditor_ )
    {
	const EM::EMObject& emobj = emeditor_->emObject();
	mkclr = emobj.preferredColor();
	if ( Math::Abs(mkclr.g()-OD::Color::Green().g())<30 )
	    lineclr = OD::Color::Red();
    }
    if ( !patchmarkers_ )
    {
	patchmarkers_ = visBase::MarkerSet::create();
	patchmarkers_->ref();
	patchmarkers_->setScreenSize(markersize_);
	addChild(patchmarkers_->osgNode());
	patchmarkers_->setDisplayTransformation( transformation_ );
    }
    if ( !patchline_ )
    {
	patchline_ = visBase::PolyLine::create();
	patchline_->ref();
	patchline_->setMaterial(new visBase::Material);
	addChild(patchline_->osgNode());
	patchline_->setDisplayTransformation( transformation_ );
    }

    OD::LineStyle lsty( OD::LineStyle::Solid, 4, lineclr );
    patchline_->setLineStyle( lsty );
    patchmarkers_->setMarkersSingleColor( mkclr );
}


void MPEEditor::setEditor( MPE::ObjectEditor* eme )
{
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );

    if ( emeditor_ )
    {
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor_->emObject());
	emobj.change.remove( movementcb );
	emobj.unRef();
	emeditor_->editpositionchange.remove( numnodescb );
	emeditor_->unRef();
    }

    emeditor_ = eme;

    if ( emeditor_ )
    {
	emeditor_->ref();
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor_->emObject());
	emobj.ref();
	emobj.change.notify( movementcb );
	emeditor_->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher_ )
	eventcatcher_->unRef();

    eventcatcher_ = nev;
    sower_->setEventCatcher( nev );

    if ( eventcatcher_ )
	eventcatcher_->ref();
}


void MPEEditor::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    for ( int idx=0; idx<draggers_.size(); idx++ )
	draggers_[idx]->setDisplayTransformation( transformation_ );

    sower_->setDisplayTransformation( nt );
    if ( patchmarkers_ )
	patchmarkers_->setDisplayTransformation( nt );
    if ( patchline_ )
	patchline_->setDisplayTransformation( nt );
}


void MPEEditor::setMarkerStyle( const MarkerStyle3D& mkstyle )
{
    for ( int idx = 0; idx<draggers_.size(); idx++ )
    {
	draggermarkers_[idx]->setMarkerStyle( mkstyle );
	draggermarkers_[idx]->setMarkersSingleColor( mkstyle.color_ );
    }

    if ( patchmarkers_ )
    {
	patchmarkers_->setMarkerStyle( mkstyle );
	patchmarkers_->setMarkersSingleColor( mkstyle.color_ );
    }

    markerstyle_ = mkstyle;
}


const MarkerStyle3D* MPEEditor::markerStyle() const
{
    return &markerstyle_;
}


void MPEEditor::setMarkerSize(float nsz)
{
    markersize_ = nsz;
    for ( int idx=0; idx<draggers_.size(); idx++ )
	draggermarkers_[idx]->setScreenSize( nsz );

    if ( patchmarkers_ )
	patchmarkers_->setScreenSize( nsz );
}


void MPEEditor::turnOnMarker( EM::PosID pid, bool on )
{
    const int mkidx = posids_.indexOf( pid );
    if ( mkidx<0 )
	return;

    draggermarkers_[mkidx]->turnOn( on );
}


bool MPEEditor::allMarkersDisplayed() const
{
    for ( int idx=0; idx<draggermarkers_.size(); idx++ )
    {
	if ( !draggermarkers_[idx]->isOn() )
	    return false;
    }

    return true;
}


EM::PosID MPEEditor::getNodePosID(int idx) const
{ return idx>=0&&idx<posids_.size() ? posids_[idx] : EM::PosID::udf(); }


bool MPEEditor::mouseClick( const EM::PosID& pid,
			    bool shift, bool alt, bool ctrl )
{
    if ( !shift && !alt && ctrl && emeditor_ )
    {
	TypeSet<EM::PosID> pids;
	emeditor_->getEditIDs(pids);
	for ( int idx=0; idx<pids.size(); idx++ )
	    emeditor_->removeEditID(pids[idx]);

	if ( emeditor_->addEditID(pid) )
	    setActiveDragger( pid );
	return true;
    }

    return false;
}


void MPEEditor::changeNumNodes( CallBacker* )
{
    TypeSet<EM::PosID> editnodes;
    if ( emeditor_ )
	emeditor_->getEditIDs( editnodes );
    else
	posids_.erase();

    TypeSet<EM::PosID> nodestoremove( posids_ );
    nodestoremove.createDifference( editnodes, false );

    if ( nodestoremove.indexOf(activedragger_)!=-1 )
	setActiveDragger( EM::PosID::udf() );

    for ( int idx=0; idx<nodestoremove.size(); idx++ )
	removeDragger( posids_.indexOf(nodestoremove[idx]) );

    TypeSet<EM::PosID> nodestoadd( editnodes );
    nodestoadd.createDifference( posids_, false );

    for ( int idx=0; idx<nodestoadd.size(); idx++ )
	addDragger( nodestoadd[idx] );
}


void MPEEditor::removeDragger( int idx )
{
    draggers_[idx]->started.remove(mCB(this,MPEEditor,dragStart));
    draggers_[idx]->motion.remove(mCB(this,MPEEditor,dragMotion));
    draggers_[idx]->finished.remove(mCB(this,MPEEditor,dragStop));
    removeChild( draggers_[idx]->osgNode() );

    draggers_.removeSingle(idx,false)->unRef();
    posids_.removeSingle(idx,false);
    draggermarkers_.removeSingle(idx,false);
}


void MPEEditor::addDragger( const EM::PosID& pid )
{
    bool translate1D = false, translate2D = false, translate3D = false;
    if ( emeditor_ )
    {
	translate1D = emeditor_->mayTranslate1D(pid);
	translate2D = emeditor_->mayTranslate2D(pid);
	translate3D = emeditor_->mayTranslate3D(pid);
    }
    else
	return;

    if ( !translate1D && !translate2D && !translate3D )
	return;

    visBase::Dragger* dragger = visBase::Dragger::create();
    dragger->ref();
    dragger->setDisplayTransformation( transformation_ );

    dragger->started.notify(mCB(this,MPEEditor,dragStart));
    dragger->motion.notify(mCB(this,MPEEditor,dragMotion));
    dragger->finished.notify(mCB(this,MPEEditor,dragStop));

    if ( translate3D )
    {
	dragger->setDraggerType( visBase::Dragger::Translate3D );
	dragger->setDefaultRotation();
    }
    else if ( translate2D )
    {
	dragger->setDraggerType( visBase::Dragger::Translate2D );
	const Coord3 defnormal( 0, 0, 1 );
	const Coord3 desnormal =
	    emeditor_->translation2DNormal(pid).normalize();
	const float dotproduct = (float) defnormal.dot(desnormal);

	Coord3 rotationaxis( 0, 0, 1 );
	float angle = 0;
	if ( !mIsEqual( dotproduct, 1, 1e-3 ) )
	{
	    const float zaxisangle =
		mCast(float, Math::Atan2( desnormal.y, desnormal.x ) );
	    Quaternion rotation( defnormal, zaxisangle );
	    rotation *= Quaternion( Coord3(0,1,0), -Math::ACos(dotproduct) );
	    rotation.getRotation( rotationaxis, angle );
	}

	dragger->setRotation( rotationaxis, angle );
    }
    else
    {
	dragger->setDraggerType( visBase::Dragger::Translate1D );
	const Coord3 defori( 1, 0, 0 );
	const Coord3 desori =
	    emeditor_->translation1DDirection( pid ).normalize().normalize();
	const float angle = (float) Math::ACos( defori.dot(desori) );
	const Coord3 axis = defori.cross(desori);
	dragger->setRotation( axis, angle );
    }

    visBase::MarkerSet* marker = visBase::MarkerSet::create();
    marker->setMarkersSingleColor( nodematerial_->getColor() );


    marker->setMarkerHeightRatio( 1.0f );
    marker->setMarkerStyle( markerstyle_ );
    marker->setMarkersSingleColor( markerstyle_.color_ );
    marker->setAutoRotateMode( visBase::MarkerSet::NO_ROTATION );
    marker->addPos( Coord3( 0, 0, 0 ) );
    marker->setMarkerResolution( 0.8f );
    dragger->setSize( markersize_ );
    dragger->setOwnShape( marker,false );

    dragger->setPos( emeditor_->getPosition(pid) );

    addChild( dragger->osgNode() );

    draggers_ += dragger;
    draggermarkers_ += marker;
    posids_ += pid;
}


void MPEEditor::nodeMovement(CallBacker* cb)
{
    if ( emeditor_ )
    {
	mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	    const int idx = posids_.indexOf( cbdata.pid0 );
	    if ( idx==-1 ) return;

	    const Coord3 pos = emeditor_->getPosition( cbdata.pid0 );
	    updateNodePos( idx, pos );
	}
	else if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert )
	{
	    for ( int idx=0; idx<posids_.size(); idx++ )
	    {
		const Coord3 pos = emeditor_->getPosition( posids_[idx] );
		updateNodePos( idx, pos );
	    }
	}
    }
}


void MPEEditor::updateNodePos( int idx, const Coord3& pos )
{
    if ( draggerinmotion_ && posids_[idx]==activedragger_ )
	return;

    draggers_[idx]->setPos( pos );
}


bool MPEEditor::clickCB( CallBacker* cb )
{
    if ( eventcatcher_->isHandled() || !isOn() )
	return true;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return true;

    int nodeidx = -1;
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( eventinfo.pickedobjids.indexOf(draggers_[idx]->id()) != -1 )
	{
	    nodeidx = idx;
	    break;
	}
    }

    if ( nodeidx==-1 )
	return true;

    if ( OD::rightMouseButton(eventinfo.buttonstate_) )
    {
	rightclicknode_ = nodeidx;
	nodeRightClick.trigger();
	eventcatcher_->setHandled();
	return false;
    }

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) )
	return true;

    return false;
}


void MPEEditor::displayPatch( const MPE::Patch* patch )
{
    setupPatchDisplay();
    if ( !patch || !patchmarkers_ || !patchline_ )
	return;

    cleanPatch();
    TypeSet<TrcKeyValue> path = patch->getPath();
    for ( int idx=0; idx<path.size(); idx++ )
    {
	const Coord3 pos = patch->seedCoord( idx );
	if ( pos.isDefined() )
	{
	    patchmarkers_->addPos( pos );
	    patchline_->addPoint( pos );
	}
    }
}


void MPEEditor::cleanPatch()
{
    if ( !patchline_ || !patchmarkers_ )
	return;

    if ( patchline_->getCoordinates()==0 && patchmarkers_->size()==0 )
	return;

    patchline_->removeAllPoints();
    patchmarkers_->clearMarkers();
    patchmarkers_->forceRedraw( true );
    patchline_->dirtyCoordinates();
}


EM::PosID MPEEditor::mouseClickDragger( const TypeSet<VisID>& path ) const
{
    if ( path.isEmpty() )
	return EM::PosID::udf();

    for ( int idx=draggers_.size()-1; idx>=0; idx-- )
    {
	if ( path.indexOf(draggers_[idx]->id()) != -1 )
	    return posids_[idx];
    }

    return EM::PosID::udf();
}


int MPEEditor::getRightClickNode() const { return rightclicknode_; }


void MPEEditor::dragStart( CallBacker* cb )
{
    const int idx = draggers_.indexOf((visBase::Dragger*) cb );
    setActiveDragger( posids_[idx] );

    if ( emeditor_ ) emeditor_->startEdit(activedragger_);
    isdragging_ = true;
    draggingStarted.trigger();
}


void MPEEditor::dragMotion( CallBacker* cb )
{
    const int idx = draggers_.indexOf( (visBase::Dragger*) cb );
    const Coord3 np = draggers_[idx]->getPos();
    draggerinmotion_ = true;
    if ( emeditor_ ) emeditor_->setPosition( np );
    draggerinmotion_ = false;
}


void MPEEditor::dragStop( CallBacker* cb )
{
    if ( emeditor_ )
    {
	const int idx = draggers_.indexOf( (visBase::Dragger*)cb );
        NotifyStopper cbstop( draggers_[idx]->motion );
	draggers_[idx]->setPos( emeditor_->getPosition( activedragger_ ) );
	emeditor_->finishEdit();
	isdragging_ = false;
    }
}


EM::PosID MPEEditor::getActiveDragger() const
{
    return isdragging_ ? activedragger_ : EM::PosID::udf();
}


void MPEEditor::setActiveDragger( const EM::PosID& pid )
{
    if ( activedragger_==pid )
	return;

    activedragger_ = pid;
}


const ObjectSet<visBase::MarkerSet>& MPEEditor::getDraggerMarkers() const
{
    return draggermarkers_;
}

} // namespace visSurvey
