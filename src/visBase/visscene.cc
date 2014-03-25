/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include <osg/Light>

#define mDefaultFactor	1
#define mDefaultUnits	200


mCreateFactoryEntry( visBase::Scene );

namespace visBase
{

Scene::Scene()
    : polygonoffset_( new PolygonOffset )
    , light_( new Light )
    , events_( *EventCatcher::create() )
    , mousedownid_( -1 )
    , blockmousesel_( false )
    , nameChanged(this)
    , osgsceneroot_( 0 )
    , camera_( 0 )
{
    light_->ref();
    light_->turnOn( false );
    light_->setAmbient( 0 );
    light_->setLightNum( 1 );
    osg::ref_ptr<osg::StateSet> stateset = osggroup_->getOrCreateStateSet();
    stateset->setAttributeAndModes(light_->osgLight() );

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
}


bool Scene::saveCurrentOffsetAsDefault() const
{
    fillOffsetPar( Settings::common() );
    return Settings::common().write( true );
}


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


void Scene::setAmbientLight( float n )
{
    //environment_->ambientIntensity.setValue( n );
}


float Scene::ambientLight() const
{
    return 0;
    //return environment_->ambientIntensity.getValue();
}


Light* Scene::getLight() const
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
    if ( blockmousesel_ || !isPickable() )
	return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( events_.isHandled() )
    {
	if ( eventinfo.type==MouseClick )
	    mousedownid_ = -1;
	return;
    }

    if ( eventinfo.dragging )
    {
	const TabletInfo* ti = TabletInfo::currentState();
	if ( ti && ti->maxPostPressDist()<5 )
	    events_.setHandled();
	else
	    mousedownid_ = -1;

	return;
    }

    if ( eventinfo.type!=MouseClick ) return;
    if ( OD::middleMouseButton(eventinfo.buttonstate_) ) return;

    if ( eventinfo.pressed )
    {
	mousedownid_ = -1;

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
	if ( !sz && mousedownid_==-1 )
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



}; // namespace visBase
