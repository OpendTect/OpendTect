/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visscene.h"

#include "settings.h"
#include "iopar.h"
#include "mouseevent.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vispolygonoffset.h"
#include "vislight.h"
#include "viscamera.h"
#include "threadwork.h"

#include <osg/Group>
#include <osg/Camera>
#include <osg/View>


#define mDefaultFactor	1
#define mDefaultUnits	200


mCreateFactoryEntry( visBase::Scene );

namespace visBase
{

static TypeSet<int> sortedfixedidxs_;

Scene::Scene()
    : polygonoffset_( new PolygonOffset )
    , light_( new Light )
    , events_( *EventCatcher::create() )
    , blockmousesel_( false )
    , nameChanged(this)
    , contextIsUp( this )
    , osgsceneroot_( 0 )
    , camera_( 0 )
{
    light_->ref();
    light_->turnOn( false );
    light_->setAmbient( 0 );
    light_->setLightNum( 1 );
    addNodeState( light_ );

    polygonoffset_->ref();

    float units = mDefaultUnits;
    float factor = mDefaultFactor;

    PtrMan<IOPar> settings = Settings::common().subselect( sKeyOffset() );
    if ( settings )
    {
	settings->get( sKeyFactor(), factor );
	settings->get( sKeyUnits(), units );
    }

    polygonoffset_->setFactor( factor );
    polygonoffset_->setUnits( units );

    events_.ref();
    events_.nothandled.notify( mCB(this,Scene,mousePickCB) );


    osgsceneroot_ = new osg::Group;
    osgsceneroot_->addChild( osgNode() );
    osgsceneroot_->addChild( events_.osgNode() );
    osgsceneroot_->ref();
    setOsgNode( osgsceneroot_ );
    polygonoffset_->attachStateSet( osgsceneroot_->getOrCreateStateSet() );

    updatequeueid_ = Threads::WorkManager::twm().addQueue(
                                Threads::WorkManager::Manual, "Scene update" );

    for ( fixedidx_=0; fixedidx_<sortedfixedidxs_.size(); fixedidx_++ )
    {
	if ( fixedidx_ != sortedfixedidxs_[fixedidx_] )
	    break;
    }
    sortedfixedidxs_.insert( fixedidx_, fixedidx_ );
}


bool Scene::saveCurrentOffsetAsDefault() const
{
    fillOffsetPar( Settings::common() );
    return Settings::common().write( true );
}


float Scene::getPolygonOffsetFactor() const
{ return polygonoffset_->getFactor(); }


float Scene::getPolygonOffsetUnits() const
{ return polygonoffset_->getUnits(); }


void Scene::setCamera( visBase::Camera* cam )
{
    if ( camera_ )
    {
        pErrMsg( "Should not be set");
        return;
    }

    camera_ = cam;
    camera_->ref();

    mAttachCB( camera_->postDraw, Scene::runUpdateQueueCB );
}


Scene::~Scene()
{
    detachAllNotifiers();

    Threads::WorkManager::twm().removeQueue( updatequeueid_, false );

    if ( camera_ )
	camera_->unRef();

    if ( osgsceneroot_ )
	osgsceneroot_->unref();

    removeAll();
    events_.nothandled.remove( mCB(this,Scene,mousePickCB) );
    events_.unRef();

    light_->unRef();

    sortedfixedidxs_.removeSingle( sortedfixedidxs_.indexOf(fixedidx_) );
}


void Scene::runUpdateQueueCB(CallBacker *)
{
    if ( !visualizationthread_ )
        setVisualizationThread( Threads::currentThread() );

    Threads::WorkManager::twm().executeQueue( updatequeueid_ );
}


void Scene::addObject( DataObject* dataobj )
{
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events_ );
    DataObjectGroup::addObject( dataobj );
}


void Scene::setBackgroundColor( const OD::Color& col )
{
    if ( camera_ )
	camera_->setBackgroundColor( col );
}


OD::Color Scene::getBackgroundColor() const
{
    return camera_ ? camera_->getBackgroundColor()
		   :  OD::Color::NoColor();
}


void Scene::setCameraLightIntensity( float value )
{
    if ( !camera_ ) return;
    osg::Light* headlight = camera_->osgCamera()->getView()->getLight();
    headlight->setDiffuse( osg::Vec4( value, value, value, 1.0 ) );
    visBase::DataObject::requestSingleRedraw();
}


float Scene::getCameraLightIntensity() const
{
    if ( !camera_ ) return 1.0f;
    const osg::Light* headlight =
	camera_->osgCamera()->getView()->getLight();
    const osg::Vec4 diffuse = headlight->getDiffuse();
    return diffuse[0];
}


void Scene::setCameraAmbientLight( float value )
{
    if ( !camera_ ) return;
    osg::Light* headlight = camera_->osgCamera()->getView()->getLight();
    headlight->setAmbient( osg::Vec4( value, value, value, 1.0 ) );
    visBase::DataObject::requestSingleRedraw();
}


float Scene::getCameraAmbientLight() const
{
    if ( !camera_ ) return 1.0f;
    const osg::Light* headlight =
	camera_->osgCamera()->getView()->getLight();
    const osg::Vec4 ambient = headlight->getAmbient();
    return ambient[0];
}


Light* Scene::getDirectionalLight() const
{
    return light_;
}


void Scene::setName( const char* newname )
{
    DataObjectGroup::setName( newname );
    nameChanged.trigger();
}


bool Scene::blockMouseSelection( bool yn )
{
    const bool res = blockmousesel_;
    blockmousesel_ = yn;
    return res;
}


EventCatcher& Scene::eventCatcher() { return events_; }


void Scene::mousePickCB( CallBacker* cb )
{
    if ( blockmousesel_ )
	return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( !isPickable() && eventinfo.pickedobjids.isEmpty() )
	return;

    if ( events_.isHandled() )
    {
	if ( eventinfo.type==MouseClick )
	    mousedownid_.setUdf();
	return;
    }

    if ( eventinfo.dragging )
    {
	const TabletInfo* ti = TabletInfo::currentState();
	if ( ti && ti->maxPostPressDist()<5 )
	    events_.setHandled();
	else
	    mousedownid_.setUdf();

	return;
    }

    if ( eventinfo.type!=MouseClick ) return;
    if ( OD::middleMouseButton(eventinfo.buttonstate_) ) return;

    if ( eventinfo.pressed )
    {
	mousedownid_.setUdf();

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    DM().getObject(eventinfo.pickedobjids[idx]);
	    if ( !dataobj )
		continue;

	    if ( dataobj->selectable() || dataobj->rightClickable() )
	    {
		mousedownid_ = eventinfo.pickedobjids[idx];
		break;
	    }
	}
    }
    else
    {
	const int sz = eventinfo.pickedobjids.size();
	if ( !sz && !mousedownid_.isValid() )
	{
	    if ( !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
		 !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		 !OD::altKeyboardButton(eventinfo.buttonstate_) )
	    {
		DM().selMan().deSelectAll();
		events_.setHandled();
	    }
	}

	bool markerclicked = false;

	for ( int idx=0; idx<sz; idx++ )
	{
	    DataObject* dataobj = DM().getObject(eventinfo.pickedobjids[idx]);
	    if ( !dataobj ) continue;

	    const bool idisok = markerclicked || mousedownid_==dataobj->id();
	    if ( !idisok ) continue;

	    if ( dataobj->rightClickable() &&
		 OD::rightMouseButton(eventinfo.buttonstate_) &&
		 dataobj->rightClicked() )
	    {
		mDynamicCastGet( visBase::MarkerSet*, emod, dataobj );
		if ( emod )
		{
		    markerclicked = true;
		    continue;
		}
		dataobj->triggerRightClick(&eventinfo);
		events_.setHandled();
	    }
	    else if ( dataobj->selectable() )
	    {
		if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
		      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		      !OD::altKeyboardButton(eventinfo.buttonstate_) )
		{
		    DM().selMan().select( mousedownid_, true );
		    events_.setHandled();
		}
		else if ( !OD::shiftKeyboardButton(eventinfo.buttonstate_)&&
		      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		      !OD::altKeyboardButton(eventinfo.buttonstate_) )
		{
		    DM().selMan().select( mousedownid_, false );
		    events_.setHandled();
		}
	    }

	    break;
	}
    }

    //Note:
    //Don't call setHandled, since that will block all other event-catchers
    //(Does not apply for OSG. Every scene has only one EventCatcher. JCG)
}


void Scene::fillOffsetPar( IOPar& par ) const
{
    IOPar offsetpar;
    offsetpar.set( sKeyFactor(), polygonoffset_->getFactor() );
    offsetpar.set( sKeyUnits(), polygonoffset_->getUnits() );
    par.mergeComp( offsetpar, sKeyOffset() );
}

} // namespace visBase
