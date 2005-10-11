/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2002
 RCS:           $Id: visscene.cc,v 1.29 2005-10-11 22:14:36 cvskris Exp $
________________________________________________________________________

-*/

#include "visscene.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"

#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoPolygonOffset.h>

mCreateFactoryEntry( visBase::Scene );

namespace visBase
{

Scene::Scene()
    : selroot( new SoGroup )
    , environment( new SoEnvironment )
    , polygonoffset( new SoPolygonOffset )
    , events( *EventCatcher::create() )
    , mousedownid( -1 )
    , blockmousesel( false )
{
    selroot->ref();

    polygonoffset->styles = SoPolygonOffset::FILLED;
    polygonoffset->factor.setValue(1);
    polygonoffset->units.setValue(2);
    selroot->addChild( polygonoffset );

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


bool Scene::blockMouseSelection( bool yn )
{
    const bool res = blockmousesel;
    blockmousesel = yn;
    return res;
}


SoNode* Scene::getInventorNode()
{
    return selroot;
}


void Scene::mousePickCB( CallBacker* cb )
{
    if ( blockmousesel )
	return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( events.isEventHandled() )
    {
	if ( eventinfo.type==MouseClick )
	    mousedownid = -1;
	return;
    }

    if ( eventinfo.type!=MouseClick ) return;
    if ( eventinfo.mousebutton==EventInfo::middleMouseButton() ) return;

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
		if ( eventinfo.mousebutton==EventInfo::rightMouseButton() &&
			dataobj->rightClicked() )
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
