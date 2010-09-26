/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeeditor.cc,v 1.42 2010-09-26 11:14:34 cvsjaap Exp $";

#include "vismpeeditor.h"

#include "bendpointfinder.h"
#include "errh.h"
#include "emeditor.h"
#include "emsurface.h"
#include "emsurfaceedgeline.h"
#include "emsurfacegeometry.h"
#include "math2.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "timefun.h"
#include "visdatagroup.h"
#include "visdragger.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "visshapescale.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"

mCreateFactoryEntry( visSurvey::MPEEditor );


namespace visSurvey
{

MPEEditor::MPEEditor()
    : visBase::VisualObjectImpl( false )
    , noderightclick( this )
    , emeditor( 0 )
    , eventcatcher( 0 )
    , transformation( 0 )
    , markersize( 3 )
    , interactionlinedisplay( 0 )
    , interactionlinerightclick( this )
    , activedragger( EM::PosID::udf() )
    , activenodematerial( 0 )
    , nodematerial( 0 )
    , isdragging( false )
    , draggerinmotion( false )
{
    nodematerial = visBase::Material::create();
    nodematerial->ref();
    nodematerial->setColor( Color(255,255,0) );

    activenodematerial = visBase::Material::create();
    activenodematerial->ref();
    activenodematerial->setColor( Color(255,0,0) );

    dummyemptysep_ = visBase::DataObjectGroup::create();
    dummyemptysep_->ref();

    sower_ = new Sower( *this );
    addChild( sower_->getInventorNode() );
}


MPEEditor::~MPEEditor()
{
    if ( interactionlinedisplay )
    {
	if ( interactionlinedisplay->rightClicked() )
	    interactionlinedisplay->rightClicked()->remove(
		mCB( this,MPEEditor,interactionLineRightClickCB));
	interactionlinedisplay->unRef();
    }


    setEditor( (MPE::ObjectEditor*) 0 );
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );

    while ( draggers.size() )
	removeDragger( 0 );

    if ( activenodematerial ) activenodematerial->unRef();
    if ( nodematerial ) nodematerial->unRef();

    dummyemptysep_->unRef();

    removeChild( sower_->getInventorNode() );
    delete sower_;
}


void MPEEditor::setEditor( MPE::ObjectEditor* eme )
{
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );

    if ( emeditor )
    {
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor->emObject());
	emobj.change.remove( movementcb );
	emobj.unRef();
	emeditor->editpositionchange.remove( numnodescb );
	emeditor->unRef();
    }

    emeditor = eme;

    if ( emeditor )
    {
	emeditor->ref();
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor->emObject());
	emobj.ref();
	emobj.change.notify( movementcb );
	emeditor->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher )
	eventcatcher->unRef();

    eventcatcher = nev;
    sower_->setEventCatcher( nev );

    if ( eventcatcher )
	eventcatcher->ref();
}


void MPEEditor::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();

    transformation = nt;
    if ( transformation )
	transformation->ref();

    for ( int idx=0; idx<draggers.size(); idx++ )
	draggers[idx]->setDisplayTransformation( transformation );

    sower_->setDisplayTransformation( nt );
}


void MPEEditor::setMarkerSize(float nsz)
{
    for ( int idx=0; idx<draggers.size(); idx++ )
	draggermarkers[idx]->setScreenSize( nsz );

    markersize = nsz;
}


void MPEEditor::turnOnMarker( EM::PosID pid, bool on )
{
    const int mkidx = posids.indexOf( pid );
    if ( mkidx<0 )
    	return;

    draggermarkers[mkidx]->turnOn( on );
}


bool MPEEditor::allMarkersDisplayed() const
{
    for ( int idx=0; idx<draggermarkers.size(); idx++ )
    {
	if ( !draggermarkers[idx]->isOn() )
	    return false;
    }
    
    return true;
}


EM::PosID MPEEditor::getNodePosID(int idx) const
{ return idx>=0&&idx<posids.size() ? posids[idx] : EM::PosID::udf(); }


bool MPEEditor::mouseClick( const EM::PosID& pid,
			    bool shift, bool alt, bool ctrl )
{
    if ( !shift && !alt && ctrl && emeditor )
    {
	TypeSet<EM::PosID> pids;
	emeditor->getEditIDs(pids);
	for ( int idx=0; idx<pids.size(); idx++ )
	    emeditor->removeEditID(pids[idx]);

	if ( emeditor->addEditID(pid) )
	    setActiveDragger( pid );
	return true;
    }

    return false;
}


void MPEEditor::changeNumNodes( CallBacker* )
{
    TypeSet<EM::PosID> editnodes;
    if ( emeditor )
	emeditor->getEditIDs( editnodes );
    else
	posids.erase();

    TypeSet<EM::PosID> nodestoremove( posids );
    nodestoremove.createDifference( editnodes, false );

    if ( nodestoremove.indexOf(activedragger)!=-1 )
	setActiveDragger( EM::PosID::udf() );

    for ( int idx=0; idx<nodestoremove.size(); idx++ )
	removeDragger( posids.indexOf(nodestoremove[idx]) );

    TypeSet<EM::PosID> nodestoadd( editnodes );
    nodestoadd.createDifference( posids, false );

    for ( int idx=0; idx<nodestoadd.size(); idx++ )
	addDragger( nodestoadd[idx] );
}


void MPEEditor::removeDragger( int idx )
{
    draggers[idx]->started.remove(mCB(this,MPEEditor,dragStart));
    draggers[idx]->motion.remove(mCB(this,MPEEditor,dragMotion));
    draggers[idx]->finished.remove(mCB(this,MPEEditor,dragStop));
    removeChild( draggers[idx]->getInventorNode() );
    draggers[idx]->unRef();
    draggersshapesep[idx]->unRef();
    draggers.remove(idx,false);
    posids.remove(idx,false);
    draggersshapesep.remove(idx,false);
    draggermarkers.remove(idx,false);
}


void MPEEditor::addDragger( const EM::PosID& pid )
{
    bool translate1D = false, translate2D = false, translate3D = false;
    if ( emeditor )
    {
	translate1D = emeditor->mayTranslate1D(pid);
	translate2D = emeditor->mayTranslate2D(pid);
	translate3D = emeditor->mayTranslate3D(pid);
    }
    else
	return;

    if ( !translate1D && !translate2D && !translate3D )
	return;
	        
    visBase::Dragger* dragger = visBase::Dragger::create();
    dragger->ref();
    dragger->setDisplayTransformation( transformation );

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
	const Coord3 desnormal = emeditor->translation2DNormal(pid).normalize();
	const float dotproduct = defnormal.dot(desnormal);
	
	Coord3 rotationaxis( 0, 0, 1 );
	float angle = 0;
	if ( !mIsEqual( dotproduct, 1, 1e-3 ) )
	{
	    const float zaxisangle = atan2( desnormal.y, desnormal.x ); 
	    Quaternion rotation( defnormal, zaxisangle );
	    rotation *= Quaternion( Coord3(0,1,0), -Math::ACos(dotproduct) );
	    rotation.getRotation( rotationaxis, angle );
	}

	dragger->setRotation( rotationaxis, angle );
	dragger->setOwnShape( dummyemptysep_, "xAxisFeedback" );
	dragger->setOwnShape( dummyemptysep_, "yAxisFeedback" );
    }
    else 
    {
	dragger->setDraggerType( visBase::Dragger::Translate1D );
	const Coord3 defori( 1, 0, 0 );
	const Coord3 desori =
	    emeditor->translation1DDirection( pid ).normalize().normalize();
	const float angle = Math::ACos( defori.dot(desori) );
	const Coord3 axis = defori.cross(desori);
	dragger->setRotation( axis, angle );
    }

    visBase::Marker* marker = visBase::Marker::create();
    marker->setMaterial( nodematerial );
    marker->setScreenSize( markersize );
    visBase::DataObjectGroup* separator = visBase::DataObjectGroup::create();
    separator->setSeparate( true );
    separator->addObject( marker );
    separator->ref();
    dragger->setOwnShape( separator, "translator" );

    visBase::ShapeScale* shapescale = visBase::ShapeScale::create();
    shapescale->ref();
    shapescale->restoreProportions(true);
    shapescale->setScreenSize(20);
    shapescale->setMinScale( 10 );
    shapescale->setMaxScale( 150 );
    shapescale->setShape( dragger->getShape("translatorActive"));
    dragger->setOwnShape( shapescale, "translatorActive" );
    shapescale->unRef();

    dragger->setPos( emeditor->getPosition(pid) );

    addChild( dragger->getInventorNode() );

    draggers += dragger;
    draggermarkers += marker;
    draggersshapesep += separator;
    posids += pid;
}



void MPEEditor::nodeMovement(CallBacker* cb)
{
    if ( emeditor )
    {
	mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	    const int idx = posids.indexOf( cbdata.pid0 );
	    if ( idx==-1 ) return;

	    const Coord3 pos = emeditor->getPosition( cbdata.pid0 );
	    updateNodePos( idx, pos );
	}
	else if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert )
	{
	    for ( int idx=0; idx<posids.size(); idx++ )
	    {
		const Coord3 pos = emeditor->getPosition( posids[idx] );
		updateNodePos( idx, pos );
	    }
	}
    }
}


void MPEEditor::updateNodePos( int idx, const Coord3& pos )
{
    if ( draggerinmotion && posids[idx]==activedragger )
	return;

    draggers[idx]->setPos( pos );
}


void MPEEditor::interactionLineRightClickCB( CallBacker* )
{ interactionlinerightclick.trigger(); }

	
bool MPEEditor::clickCB( CallBacker* cb )
{
    if ( eventcatcher->isHandled() || !isOn() )
	return true;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return true;

    int nodeidx = -1;
    for ( int idx=0; idx<draggers.size(); idx++ )
    {
	if ( eventinfo.pickedobjids.indexOf(draggers[idx]->id()) != -1 )
	{
	    nodeidx = idx;
	    break;
	}
    }

    if ( nodeidx==-1 )
	return true;

    if ( OD::rightMouseButton(eventinfo.buttonstate_) )
    {
	rightclicknode = nodeidx;
	noderightclick.trigger();
	eventcatcher->setHandled();
	return false;
    }

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) )
	return true;

    return false;
}


EM::PosID MPEEditor::mouseClickDragger( const TypeSet<int>& path ) const
{
    for ( int idx=draggers.size()-1; idx>=0; idx-- )
    {
	if ( path.indexOf(draggers[idx]->id()) != -1 )
	    return posids[idx];
    }

    return EM::PosID::udf();
}


int MPEEditor::getRightClickNode() const { return rightclicknode; }


void MPEEditor::dragStart( CallBacker* cb )
{
    const int idx = draggers.indexOf((visBase::Dragger*) cb );
    setActiveDragger( posids[idx] );
    if ( emeditor ) emeditor->startEdit(activedragger);
    isdragging = true;
}


void MPEEditor::dragMotion( CallBacker* cb )
{
    const int idx = draggers.indexOf( (visBase::Dragger*) cb );
    const Coord3 np = draggers[idx]->getPos();
    draggerinmotion = true;
    if ( emeditor ) emeditor->setPosition( np );
    draggerinmotion = false;
}


void MPEEditor::dragStop( CallBacker* cb )
{
    if ( emeditor ) 
    { 
	const int idx = draggers.indexOf( (visBase::Dragger*) cb );
        NotifyStopper cbstop( draggers[idx]->motion );
	draggers[idx]->setPos( emeditor->getPosition( activedragger ) );
	emeditor->finishEdit();
	isdragging = false;
    }
}


void MPEEditor::setActiveDragger( const EM::PosID& pid )
{
    if ( emeditor ) emeditor->restartInteractionLine(pid);

    if ( activedragger==pid )
	return;

/*
    int idx = posids.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers[idx]->setMaterial( nodematerial );
	*/

    activedragger = pid;
/*
    idx = posids.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers[idx]->setMaterial( activenodematerial );
	*/
}


void MPEEditor::setupInteractionLineDisplay()
{
    const EM::EdgeLineSet* edgelineset = emeditor
	? emeditor->getInteractionLine() : 0;

    if ( !edgelineset )
	return;

    if ( !interactionlinedisplay )
    {
	interactionlinedisplay = EdgeLineSetDisplay::create();
	interactionlinedisplay->setDisplayTransformation( transformation );
	interactionlinedisplay->ref();
	interactionlinedisplay->setConnect(true);
	interactionlinedisplay->setShowDefault(true);
	interactionlinedisplay->getMaterial()->setColor(Color(255,255,0),0);

	addChild( interactionlinedisplay->getInventorNode() );
	if ( interactionlinedisplay->rightClicked() )
	    interactionlinedisplay->rightClicked()->notify(
		mCB( this,MPEEditor,interactionLineRightClickCB));
    }

    interactionlinedisplay->setEdgeLineSet(edgelineset);
} 


void MPEEditor::extendInteractionLine( const EM::PosID& pid )
{
    setupInteractionLineDisplay();
    if ( !interactionlinedisplay || activedragger.subID()==-1 )
	return;

    emeditor->interactionLineInteraction(pid);
}


Sower::Sower( const MPEEditor& mpeed )
    : visBase::VisualObjectImpl( false )
    , editor_( mpeed )
    , eventcatcher_( 0 )
    , mode_( Idle )
    , sowingline_( visBase::PolyLine::create() )
    , reversesowingorder_( false )
    , alternatesowingorder_( false )
    , linelost_( false )
    , singleseeded_( true )
    , curpid_( EM::PosID::udf() )
    , curpidstamp_( mUdf(int) )
{
    sowingline_->ref();
    addChild( sowingline_->getInventorNode() );
    sowingline_->setMaterial( visBase::Material::create() );

    setIfDragInvertMask( false );
    setSequentSowMask();
    setLaserMask();
    setEraserMask();
}


Sower::~Sower()
{
    removeChild( sowingline_->getInventorNode() );
    sowingline_->unRef();
    deepErase( eventlist_ );
}


void Sower::reverseSowingOrder( bool yn )
{ reversesowingorder_ = yn; }


void Sower::alternateSowingOrder( bool yn )
{ alternatesowingorder_ = yn; }


void Sower::setDisplayTransformation( visBase::Transformation* transformation )
{ sowingline_->setDisplayTransformation( transformation ); }


void Sower::setEventCatcher( visBase::EventCatcher* eventcatcher )
{ eventcatcher_ = eventcatcher; }


#define mReturnHandled( yn ) \
{ \
    if ( yn && eventcatcher_ ) eventcatcher_->setHandled(); \
    return yn; \
}

bool Sower::activate( const Color& color, const visBase::EventInfo& eventinfo )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    Scene* scene = STM().currentScene();
    if ( scene && scene->getPolySelection()->getSelectionType() !=
	    					visBase::PolygonSelection::Off )
	mReturnHandled( false );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	mReturnHandled( false );

    mode_ = Furrowing;
    if ( !accept(eventinfo) )
	mReturnHandled( false );

    sowingline_->getMaterial()->setColor( color );
    sowingline_->turnOn( true );

    mReturnHandled( true );
}


Coord3 Sower::pivotPos() const
{
    if ( mode_<FirstSowing || eventlist_.isEmpty() )
	return Coord3::udf();

    Coord3 sum = eventlist_[0]->displaypickedpos;
    sum += eventlist_[eventlist_.size()-1]->displaypickedpos;
    return 0.5*sum;
}


bool Sower::moreToSow() const
{ return mode_>=FirstSowing && bendpoints_.size()>1; }


void Sower::stopSowing()
{ bendpoints_.erase(); }


bool Sower::accept( const visBase::EventInfo& eventinfo )
{
    if ( eventinfo.tabletinfo )
	return acceptTablet( eventinfo );

    return acceptMouse( eventinfo );
}


bool Sower::acceptMouse( const visBase::EventInfo& eventinfo )
{
    if ( mode_ == Idle &&
	 eventinfo.type==visBase::MouseClick && !eventinfo.pressed )
    {
	const EM::PosID pid = editor_.mouseClickDragger(eventinfo.pickedobjids);
	if ( pid.isUdf() )
	    mReturnHandled( true );
    }

    if ( mode_ != Furrowing )
	mReturnHandled( false );

    if ( eventinfo.type == visBase::Keyboard )
	mReturnHandled( true );

    const int sz = eventlist_.size();
    if ( eventinfo.type==visBase::MouseMovement || eventinfo.pressed )
    {
	if ( sz && eventinfo.pickedobjids!=eventlist_[0]->pickedobjids )
	{
	    if ( eventinfo.worldpickedpos.isDefined() && !linelost_ )
		sowingline_->addPoint( eventinfo.worldpickedpos );
	    else
		linelost_ = true;

	    mReturnHandled( true );
	}

	linelost_ = false;
	sowingline_->addPoint( eventinfo.worldpickedpos );

	if ( !sz )
	    singleseeded_ = true;
	else if ( eventinfo.mousepos.distTo(eventlist_[0]->mousepos) > 5 )
	    singleseeded_ = false;

	eventlist_ += new visBase::EventInfo( eventinfo );
	mousecoords_ += eventinfo.mousepos;
	mReturnHandled( true );
    }

    if ( !sz )
	mReturnHandled( true );

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    int butstate = eventlist_[0]->buttonstate_;
    if ( !singleseeded_ )
	butstate ^= ifdraginvertmask_;

    eventlist_[0]->buttonstate_ = (OD::ButtonState) butstate;
    butstate &= sequentsowmask_;

    for ( int idx=sz-1; idx>0; idx--)
    {
	if ( singleseeded_ )
	{
	    eventlist_.remove( idx );
	    mousecoords_.remove( idx );
	}
	else
	{
	    eventlist_[idx]->type = visBase::MouseClick;
	    eventlist_[idx]->buttonstate_ = (OD::ButtonState) butstate;
	}
    }

    BendPointFinder2D bpfinder ( mousecoords_, 2 );
    bpfinder.execute( true );
    bendpoints_ = bpfinder.bendPoints();
    if ( reversesowingorder_ )
	bendpoints_.reverse();

    mode_ = FirstSowing;
    while ( bendpoints_.size() )
    {
	int eventidx = bendpoints_[0];
	for ( int yn=1; yn>=0; yn-- )
	{
	    eventlist_[eventidx]->pressed = yn;
	    if ( eventcatcher_ )
		eventcatcher_->reHandle( *eventlist_[eventidx] );
	}

	bendpoints_.remove( 0 );
	if ( alternatesowingorder_ )
	    bendpoints_.reverse();

	mode_ = SequentSowing;
    }

    reset();
    mReturnHandled( true );
}


void Sower::reset()
{
    sowingline_->turnOn( false );
    for ( int idx=sowingline_->size()-1; idx>=0; idx-- )
	sowingline_->removePoint( idx );

    deepErase( eventlist_ );
    mousecoords_.erase();

    mode_ = Idle;
}


bool Sower::acceptTablet( const visBase::EventInfo& eventinfo )
{
    if ( !eventinfo.tabletinfo )
	mReturnHandled( false );

    const EM::PosID pid = editor_.mouseClickDragger( eventinfo.pickedobjids );
    if ( pid != curpid_ )
    {
	curpidstamp_ = Time::getMilliSeconds();
	curpid_ = pid;
    }

    if ( eventinfo.tabletinfo->pointertype_ == TabletInfo::Eraser )
    {
	if ( !pid.isUdf() )
	    return acceptEraser( eventinfo );

	mReturnHandled( true );
    }

    if ( mode_==Idle && eventinfo.type==visBase::MouseMovement &&
	 !pid.isUdf() && !mIsUdf(curpidstamp_) &&
	 Time::passedSince(curpidstamp_) > 300 )
    {
	curpidstamp_ = mUdf(int);
	return acceptLaser( eventinfo );
    }

    if ( !pid.isUdf() && mode_==Furrowing && singleseeded_ )
    {
	reset();
	mReturnHandled( false );
    }

    return acceptMouse( eventinfo );
}


bool Sower::acceptLaser( const visBase::EventInfo& eventinfo )
{
    if ( mode_!=Idle )
	mReturnHandled( false );

    mode_ = Lasering;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | lasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


bool Sower::acceptEraser( const visBase::EventInfo& eventinfo )
{
    if ( mode_!=Idle )
	mReturnHandled( false );

    if ( eventinfo.type==visBase::MouseMovement &&
	 !eventinfo.tabletinfo->pressure_ )
	mReturnHandled( false );

    mode_ = Erasing;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | erasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


void Sower::setSequentSowMask( bool yn, OD::ButtonState mask )
{ sequentsowmask_ = yn ? mask : OD::ButtonState(~OD::NoButton); }


void Sower::setIfDragInvertMask( bool yn, OD::ButtonState mask )
{ ifdraginvertmask_ = yn ? mask : OD::NoButton; }


void Sower::setLaserMask( bool yn, OD::ButtonState mask )
{ lasermask_ = yn ? mask : OD::NoButton; }


void Sower::setEraserMask( bool yn, OD::ButtonState mask )
{ erasermask_ = yn ? mask : OD::NoButton; }


}; //namespace
