/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.11 2002-04-10 07:49:49 kristofer Exp $";

#include "vissurvpickset.h"
#include "visevent.h"
#include "vissceneobjgroup.h"
#include "vissurvscene.h"
#include "position.h"
#include "viscube.h"
#include "geompos.h"
#include "color.h"

visSurvey::PickSetDisplay::PickSetDisplay()
    : group( visBase::SceneObjectGroup::create(true) )
    , eventcatcher( visBase::EventCatcher::create(visBase::MouseClick) )
    , inlsz( 5 )
    , crlsz( 5 )
    , tsz( 0.05 )
    , addedpoint( this )
    , removedpoint( this )
    , selected( false )
{
    eventcatcher->ref();
    addChild( eventcatcher->getData() );

    eventcatcher->eventhappened.notify( mCB(this,visSurvey::PickSetDisplay,pickCB ));

    group->ref();
    addChild( group->getData() );
}


visSurvey::PickSetDisplay::~PickSetDisplay()
{
    eventcatcher->eventhappened.remove( mCB(this,visSurvey::PickSetDisplay,pickCB ));
    removeChild( eventcatcher->getData() );
    eventcatcher->unRef();
    removeChild( group->getData() );
    group->unRef();
}


int visSurvey::PickSetDisplay::nrPicks() const
{
    return group->size();
}


Geometry::Pos visSurvey::PickSetDisplay::getPick( int idx ) const
{

    mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
    if ( cube )
    {
	return cube->centerPos();
    }

    Geometry::Pos res(mUndefValue,mUndefValue,mUndefValue);

    return res;
}


void visSurvey::PickSetDisplay::addPick( const Geometry::Pos& pos )
{
    visBase::Cube* cube = visBase::Cube::create();
    cube->setCenterPos( pos );
    cube->setWidth( Geometry::Pos( inlsz, crlsz, tsz) );
    cube->setMaterial( 0 );

    group->addObject( cube );
}


void visSurvey::PickSetDisplay::setSize( float inl, float crl, float t )
{
    inlsz = inl; crlsz = crl; tsz = t;

    Geometry::Pos nsz( inl, crl, t );

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	cube->setWidth( nsz );
    }
}


void visSurvey::PickSetDisplay::removePick( const Geometry::Pos& pos )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

//	TODO: remove cube nearest to pos.
	if ( cube->centerPos() == pos )
	{
	    group->removeObject( idx );
	    return;
	}
    }
}


void visSurvey::PickSetDisplay::removeAll()
{
    group->removeAll();
}


void visSurvey::PickSetDisplay::pickCB(CallBacker* cb)
{
    if ( !selected ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseClick ) return;
    if ( eventinfo.mousebutton ) return;

    if ( eventinfo.pressed )
    {
	mousepressid = -1;
	mousepressposition.x = mUndefValue;
	mousepressposition.y = mUndefValue;
	mousepressposition.z = mUndefValue;

	if ( eventinfo.ctrl )
	{
	    mousepressid = eventinfo.pickedobjid;
	    if ( mousepressid!=-1 )
	    {
		if ( group->getFirstIdx( mousepressid ) == -1 )
		    mousepressid = -1;
	    }

	    eventcatcher->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjid>0 )
	    {
		mousepressposition = eventinfo.pickedpos;
	    }
	    
	    eventcatcher->eventIsHandled();
	}
    }
    else 
    {
	if ( eventinfo.ctrl )
	{
	    if ( mousepressid==eventinfo.pickedobjid )
	    if ( mousepressid!=-1 )
	    {
		int removeidx = group->getFirstIdx(mousepressid);
		if ( removeidx != -1 )
		{
		    group->removeObject( removeidx );
		}
	    }

	    eventcatcher->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjid==mousepressid )
	    {
		Geometry::Pos newpos = eventinfo.pickedpos;

		if ( mIS_ZERO( newpos.x-mousepressposition.x ) &&
		     mIS_ZERO( newpos.y-mousepressposition.y ) &&
		     mIS_ZERO( newpos.z-mousepressposition.z ) )
		{
		    addPick( newpos );
		}
	    }

	    eventcatcher->eventIsHandled();
	}
    }
}
	    
