/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.25 2002-05-03 13:38:45 kristofer Exp $";

#include "vissurvpickset.h"
#include "visevent.h"
#include "visdataman.h"
#include "visplanedatadisplay.h"
#include "vissceneobjgroup.h"
#include "position.h"
#include "viscube.h"
#include "geompos.h"
#include "color.h"

mCreateFactoryEntry( visSurvey::PickSetDisplay );

visSurvey::PickSetDisplay::PickSetDisplay( )
    : group( visBase::SceneObjectGroup::create() )
    , eventcatcher( visBase::EventCatcher::create() )
    , xsz( 50 )
    , ysz( 50 )
    , zsz( 50 )
    , changed( this )
    , VisualObjectImpl( true )
    , showall(true)
{
    eventcatcher->ref();
    eventcatcher->setEventType(visBase::MouseClick);
    addChild( eventcatcher->getData() );

    eventcatcher->eventhappened.notify(
	    mCB(this,visSurvey::PickSetDisplay,pickCB ));

    SPM().appvelchange.notify(	mCB( this, visSurvey::PickSetDisplay,
				updateCubeSz ));

    group->ref();
    addChild( group->getData() );
   
}


visSurvey::PickSetDisplay::~PickSetDisplay()
{

    eventcatcher->eventhappened.remove(
	    mCB(this,visSurvey::PickSetDisplay,pickCB ));
    removeChild( eventcatcher->getData() );
    eventcatcher->unRef();
    removeChild( group->getData() );
    group->unRef();

    SPM().appvelchange.remove(	mCB( this, visSurvey::PickSetDisplay,
				updateCubeSz ));
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
    cube->setWidth( Geometry::Pos( xsz, ysz, zsz/SPM().getAppVel()*2) );
    cube->setMaterial( 0 );

    group->addObject( cube );
    changed.trigger();
}


void visSurvey::PickSetDisplay::showAll(bool yn)
{
    showall = yn;
    if ( !showall ) return;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	cube->turnOn( true );
    }
}


void visSurvey::PickSetDisplay::filterPicks( ObjectSet<SurveyObject>& objs,
					     float dist )
{
    if ( showall ) return;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	Geometry::Pos pos = SPM().coordXYT2Display(cube->centerPos());
	cube->turnOn( false );
	for ( int idy=0; idy<objs.size(); idy++ )
	{
	    if ( objs[idy]->calcDist( pos )< dist )
	    {
		cube->turnOn(true);
		break;
	    }
	}
    }
}


void visSurvey::PickSetDisplay::setSize( float x, float y, float z )
{
    xsz = x; ysz = y; zsz = z;
    updateCubeSz( 0 );
}


void visSurvey::PickSetDisplay::removePick( const Geometry::Pos& pos )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	if ( cube->centerPos() == pos )
	{
	    group->removeObject( idx );
	    changed.trigger();
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
    if ( !isSelected() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseClick ) return;
    if ( eventinfo.mousebutton ) return;

    if ( eventinfo.pressed )
    {
	mousepressid = -1;
	mousepressposition.x = mUndefValue;
	mousepressposition.y = mUndefValue;
	mousepressposition.z = mUndefValue;

	if ( eventinfo.pickedobjids.size() )
	{
	    mousepressid = eventinfo.pickedobjids[0];
	    mousepressposition = eventinfo.pickedpos;
	}

	eventcatcher->eventIsHandled();
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventinfo.pickedobjids[0]==mousepressid )
	    {
		int removeidx = group->getFirstIdx(mousepressid);
		if ( removeidx != -1 )
		{
		    group->removeObject( removeidx );
		    changed.trigger();
		}
	    }

	    eventcatcher->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventinfo.pickedobjids[0]==mousepressid )
	    {
		const int sz = eventinfo.pickedobjids.size();
		bool validpicksurface = false;

		for ( int idx=0; idx<sz; idx++ )
		{
		    const DataObject* pickedobj =
			visBase::DM().getObj(eventinfo.pickedobjids[idx]);

		    if ( typeid(*pickedobj)==
			    typeid(visSurvey::PlaneDataDisplay) )
		    {
			validpicksurface = true;
			break;
		    }
		}

		if ( validpicksurface )
		{
		    Geometry::Pos newpos = eventinfo.pickedpos;

		    if ( mIS_ZERO( newpos.x-mousepressposition.x ) &&
			 mIS_ZERO( newpos.y-mousepressposition.y ) &&
			 mIS_ZERO( newpos.z-mousepressposition.z ) )
		    {
			Geometry::Pos realpos = SPM().coordDispl2XYT( newpos );
			addPick( realpos );
		    }
		}
	    }

	    eventcatcher->eventIsHandled();
	}
    }
}


void visSurvey::PickSetDisplay::updateCubeSz( CallBacker* cb )
{
    Geometry::Pos nsz( xsz, ysz, zsz/SPM().getAppVel()*2);

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	cube->setWidth( nsz );
    }
}


	    
