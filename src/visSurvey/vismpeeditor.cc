/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vismpeeditor.h"

#include "emeditor.h"
#include "emsurface.h"
#include "emsurfaceedgeline.h"
#include "emsurfacegeometry.h"
#include "math2.h"
#include "visdatagroup.h"
#include "visdragger.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vissower.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::MPEEditor );


namespace visSurvey
{

MPEEditor::MPEEditor()
    : visBase::VisualObjectImpl( false )
    , noderightclick( this )
    , emeditor_( 0 )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , markersize_( 10 )
    , interactionlinedisplay_( 0 )
    , interactionlinerightclick( this )
    , activedragger_( EM::PosID::udf() )
    , activenodematerial_( 0 )
    , nodematerial_( 0 )
    , isdragging_( false )
    , draggerinmotion_( false )
{
    nodematerial_ = new visBase::Material;
    nodematerial_->ref();
    nodematerial_->setColor( Color(255,255,0) );

    activenodematerial_ = new visBase::Material;
    activenodematerial_->ref();
    activenodematerial_->setColor( Color(255,0,0) );

    sower_ = new Sower( this );
    addChild( sower_->osgNode() );
}


MPEEditor::~MPEEditor()
{
    if ( interactionlinedisplay_ )
    {
	if ( interactionlinedisplay_->rightClicked() )
	    interactionlinedisplay_->rightClicked()->remove(
		mCB( this,MPEEditor,interactionLineRightClickCB));
	interactionlinedisplay_->unRef();
    }


    setEditor( (MPE::ObjectEditor*) 0 );
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );

    while ( draggers_.size() )
	removeDragger( 0 );

    if ( activenodematerial_ ) activenodematerial_->unRef();
    if ( nodematerial_ ) nodematerial_->unRef();

    removeChild( sower_->osgNode() );
    delete sower_;
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
}


void MPEEditor::setMarkerSize(float nsz)
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
	draggermarkers_[idx]->setScreenSize( nsz );

    markersize_ = nsz;
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
	    const float zaxisangle = (float) atan2( desnormal.y, desnormal.x ); 
	    Quaternion rotation( defnormal, zaxisangle );
	    rotation *= Quaternion( Coord3(0,0,1), -Math::ACos(dotproduct) );
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


    MarkerStyle3D markerstyle;
    marker->setMarkerHeightRatio( 1.0f );
    markerstyle = MarkerStyle3D::Cube;
    markerstyle.size_ = (int)markersize_;
    marker->setMarkerStyle( markerstyle );
    marker->setMinimumScale( 0 );
    marker->setMaximumScale( 25.5f );
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


void MPEEditor::interactionLineRightClickCB( CallBacker* )
{ interactionlinerightclick.trigger(); }

	
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
	noderightclick.trigger();
	eventcatcher_->setHandled();
	return false;
    }

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) )
	return true;

    return false;
}


EM::PosID MPEEditor::mouseClickDragger( const TypeSet<int>& path ) const
{
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
	const int idx = draggers_.indexOf( (visBase::Dragger*) cb );
        NotifyStopper cbstop( draggers_[idx]->motion );
	draggers_[idx]->setPos( emeditor_->getPosition( activedragger_ ) );
	emeditor_->finishEdit();
	isdragging_ = false;
    }
}


void MPEEditor::setActiveDragger( const EM::PosID& pid )
{
    if ( emeditor_ ) emeditor_->restartInteractionLine(pid);

    if ( activedragger_==pid )
	return;

/*
    int idx = posids_.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers_[idx]->setMaterial( nodematerial );
	*/

    activedragger_ = pid;
/*
    idx = posids_.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers_[idx]->setMaterial( activenodematerial );
	*/
}


void MPEEditor::setupInteractionLineDisplay()
{
    const EM::EdgeLineSet* edgelineset = emeditor_
	? emeditor_->getInteractionLine() : 0;

    if ( !edgelineset )
	return;

    if ( !interactionlinedisplay_ )
    {
	interactionlinedisplay_ = EdgeLineSetDisplay::create();
	interactionlinedisplay_->setDisplayTransformation( transformation_ );
	interactionlinedisplay_->ref();
	interactionlinedisplay_->setConnect(true);
	interactionlinedisplay_->setShowDefault(true);
	interactionlinedisplay_->getMaterial()->setColor(Color(255,255,0),0);

	addChild( interactionlinedisplay_->osgNode() );
	if ( interactionlinedisplay_->rightClicked() )
	    interactionlinedisplay_->rightClicked()->notify(
		mCB( this,MPEEditor,interactionLineRightClickCB));
    }

    interactionlinedisplay_->setEdgeLineSet(edgelineset);
} 


void MPEEditor::extendInteractionLine( const EM::PosID& pid )
{
    setupInteractionLineDisplay();
    if ( !interactionlinedisplay_ || activedragger_.subID()==-1 )
	return;

    emeditor_->interactionLineInteraction(pid);
}



}; //namespace
