
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.14 2004-04-14 09:42:03 nanne Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoEnvironment.h"

mCreateFactoryEntry( visBase::Scene );

visBase::Scene::Scene()
    : selroot( new SoGroup )
    , environment( new SoEnvironment )
    , mouseevents( *EventCatcher::create() )
    , mousedownid( -1 )
{

    selroot->ref();
    selroot->addChild( environment );
    selroot->addChild( DataObjectGroup::getInventorNode() );
    selroot->addChild( mouseevents.getInventorNode() );
    mouseevents.setEventType( visBase::MouseClick );
    mouseevents.eventhappened.notify( mCB(this,visBase::Scene,mousePickCB) );
}


visBase::Scene::~Scene()
{
    removeAll();
    mouseevents.eventhappened.remove( mCB(this,visBase::Scene,mousePickCB) );

    selroot->unref();
}



void visBase::Scene::setAmbientLight( float n )
{
    environment->ambientIntensity.setValue( n );
}


float visBase::Scene::ambientLight() const
{
    return environment->ambientIntensity.getValue();
}


SoNode* visBase::Scene::getInventorNode()
{
    return selroot;
}


void visBase::Scene::mousePickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( mouseevents.isEventHandled() ) return;

    if ( eventinfo.type != visBase::MouseClick ) return;
    if ( eventinfo.mousebutton > 1 ) return;

    if ( eventinfo.pressed )
    {
	mousedownid = -1;

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);
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
		visBase::DM().selMan().deSelectAll();
	}

	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	    if ( dataobj->selectable() && mousedownid==dataobj->id() )
	    {
		if ( eventinfo.mousebutton == 1 )
		    visBase::DM().selMan().rightClicked( mousedownid );

		if ( eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		    visBase::DM().selMan().select( mousedownid, true );
		else if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl)
		    visBase::DM().selMan().select( mousedownid, false );

		break;
	    }
	}
    }


    mouseevents.eventIsHandled();
}

