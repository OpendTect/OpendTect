
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.22 2005-02-04 14:31:34 kristofer Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoEnvironment.h"

namespace visBase
{

mCreateFactoryEntry( Scene );

Scene::Scene()
    : selroot( new SoGroup )
    , environment( new SoEnvironment )
    , events( *EventCatcher::create() )
    , mousedownid( -1 )
{
    selroot->ref();
    selroot->addChild( environment );
    events.ref();
    selroot->addChild( events.getInventorNode() );
    selroot->addChild( DataObjectGroup::getInventorNode() );
    events.nothandled.notify( mCB(this,Scene,mousePickCB) );
}


Scene::~Scene()
{
    removeAll();
    events.nothandled.remove( mCB(this,Scene,mousePickCB) );
    events.unRef();

    selroot->unref();
}


void Scene::addObject( DataObject* dataobj )
{
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events );
    DataObjectGroup::addObject( dataobj );
}


void Scene::insertObject( int idx, DataObject* dataobj )
{
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events );
    DataObjectGroup::insertObject( idx, dataobj );
}


void Scene::setAmbientLight( float n )
{
    environment->ambientIntensity.setValue( n );
}


float Scene::ambientLight() const
{
    return environment->ambientIntensity.getValue();
}


SoNode* Scene::getInventorNode()
{
    return selroot;
}


void Scene::mousePickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( events.isEventHandled() ) return;

    if ( eventinfo.type != MouseClick ) return;
    if ( eventinfo.mousebutton > 1 ) return;

    if ( eventinfo.pressed )
    {
	mousedownid = -1;

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    DM().getObject(eventinfo.pickedobjids[idx]);
	    if ( dataobj->selectable() )
	    {
		mousedownid = eventinfo.pickedobjids[idx];
		break;
	    }
	}
    }
    else
    {
	const int sz = eventinfo.pickedobjids.size();
	if ( !sz && mousedownid==-1 )
	{
	    if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		DM().selMan().deSelectAll();
	}

	for ( int idx=0; idx<sz; idx++ )
	{
	    DataObject* dataobj =
			    DM().getObject(eventinfo.pickedobjids[idx]);

	    if ( dataobj->selectable() && mousedownid==dataobj->id() )
	    {
		if ( eventinfo.mousebutton == 1 && dataobj->rightClicked() )
		    dataobj->triggerRightClick(&eventinfo);
		if ( eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		    DM().selMan().select( mousedownid, true );
		else if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl)
		    DM().selMan().select( mousedownid, false );

		break;
	    }
	}
    }

    //Note:
    //Don't call setHandled, since that will block all other event-catchers
}


}; // namespace visBase
