
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.7 2002-04-10 08:50:09 kristofer Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoEnvironment.h"

visBase::Scene::Scene()
    : SceneObjectGroup( true )
    , selroot( new SoGroup )
    , environment( new SoEnvironment )
    , mouseevents( *EventCatcher::create( visBase::MouseClick) )
    , mousedownid( -1 )
{
    selroot->ref();
    selroot->addChild( environment );
    selroot->addChild( SceneObjectGroup::getData() );
    selroot->addChild( mouseevents.getData() );

    mouseevents.eventhappened.notify( mCB( this, visBase::Scene, mousePickCB ));
}


visBase::Scene::~Scene()
{
    removeAll();
    mouseevents.eventhappened.remove( mCB( this, visBase::Scene, mousePickCB ));

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


SoNode* visBase::Scene::getData()
{
    return selroot;
}


void visBase::Scene::mousePickCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );
    if ( mouseevents.isEventHandled() ) return;

    if ( eventinfo.type != visBase::MouseClick ) return;
    if ( eventinfo.mousebutton ) return;

    if ( eventinfo.pressed )
    {
	mousedownid = -1;

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
				visBase::DM().getObj(eventinfo.pickedobjids[0]);
	    if ( dataobj->selectable() )
	    {
		mousedownid = eventinfo.pickedobjids[0];
		break;
	    }
	}
    }
    else
    {
	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	    if ( dataobj->selectable() &&mousedownid==dataobj->id())
	    {
		if ( eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		    visBase::DM().selMan().select(mousedownid,true);
		else if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl)
		    visBase::DM().selMan().select(mousedownid,false);

		break;
	    }
	}
    }


    mouseevents.eventIsHandled();
}

