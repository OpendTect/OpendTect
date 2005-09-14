/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.1 2005-09-14 08:25:22 cvskris Exp $";

#include "vismpeseedcatcher.h"

#include "visdataman.h"
#include "visevent.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher );


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : click( this )
    , eventcatcher( 0 )
    , transformation( 0 )
    , eventinfo_( 0 )
{ }


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
}


void MPEClickCatcher::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher->unRef();
    }

    eventcatcher = nev;

    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher->ref();
    }
}


void MPEClickCatcher::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();

    transformation = nt;
    if ( transformation )
	transformation->ref();
}


void MPEClickCatcher::clickCB( CallBacker* cb )
{
    if ( eventcatcher->isEventHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return;

    if ( eventinfo.shift || eventinfo.alt || eventinfo.ctrl )
	return;

    if ( eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	mDynamicCastGet( PlaneDataDisplay*, plane,
			 visBase::DM().getObject(eventinfo.pickedobjids[idx]) );
	if ( plane )
	{
	    eventinfo_ = &eventinfo;
	    click.trigger();
	    eventinfo_ = 0;
	    eventcatcher->eventIsHandled();
	}
    }
}


}; //namespce
