/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.30 2002-07-29 07:35:03 kristofer Exp $";

#include "vissurvpickset.h"
#include "visevent.h"
#include "visdataman.h"
#include "visplanedatadisplay.h"
#include "vissceneobjgroup.h"
#include "vismaterial.h"
#include "position.h"
#include "vismarker.h"
#include "geompos.h"
#include "color.h"
#include "iopar.h"

mCreateFactoryEntry( visSurvey::PickSetDisplay );

const char* visSurvey::PickSetDisplay::grpstr = "Group";
const char* visSurvey::PickSetDisplay::showallstr = "Show all";
const char* visSurvey::PickSetDisplay::shapestr = "Shape";

visSurvey::PickSetDisplay::PickSetDisplay( )
    : group( visBase::SceneObjectGroup::create() )
    , eventcatcher( visBase::EventCatcher::create() )
    , initsz( 5 )
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
				updatePickSz ));

    group->ref();
    addChild( group->getData() );
    picksz = initsz;
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
				updatePickSz ));
}


int visSurvey::PickSetDisplay::nrPicks() const
{
    return group->size();
}


Geometry::Pos visSurvey::PickSetDisplay::getPick( int idx ) const
{

    mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
    if ( marker )
    {
	return marker->centerPos();
    }

    Geometry::Pos res(mUndefValue,mUndefValue,mUndefValue);

    return res;
}


void visSurvey::PickSetDisplay::addPick( const Geometry::Pos& pos )
{
    visBase::Marker* marker = visBase::Marker::create();
    marker->setCenterPos( pos );
    marker->setScale( Geometry::Pos(1, 1, 2/SPM().getAppVel()) );
    marker->setSize( picksz );
    marker->setType( (visBase::Marker::Type)getType() );
    marker->setMaterial( 0 );

    group->addObject( marker );
    changed.trigger();
}


void visSurvey::PickSetDisplay::showAll(bool yn)
{
    showall = yn;
    if ( !showall ) return;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	marker->turnOn( true );
    }
}


void visSurvey::PickSetDisplay::filterPicks( ObjectSet<SurveyObject>& objs,
					     float dist )
{
    if ( showall ) return;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	Geometry::Pos pos = SPM().coordXYT2Display(marker->centerPos());
	marker->turnOn( false );
	for ( int idy=0; idy<objs.size(); idy++ )
	{
	    if ( objs[idy]->calcDist( pos )< dist )
	    {
		marker->turnOn(true);
		break;
	    }
	}
    }
}


void visSurvey::PickSetDisplay::setSize( float x )
{
    picksz = x; 
    updatePickSz( 0 );
}


void visSurvey::PickSetDisplay::setColor( const Color& col )
{
    (this)->getMaterial()->setColor( col );
}


const Color& visSurvey::PickSetDisplay::getColor() const
{
    return (this)->getMaterial()->getColor();
}


void visSurvey::PickSetDisplay::setType( int tp )
{
    if ( tp < 0 ) tp = 0;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;
	marker->setType( (visBase::Marker::Type)tp );
    }
}


int visSurvey::PickSetDisplay::getType() const
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;
	return (int)marker->getType();
    }

    return -1;
}


void visSurvey::PickSetDisplay::getTypeNames( TypeSet<char*>& strs )
{
    strs += "Cube";
    strs += "Cone";
    strs += "Cylinder";
    strs += "Sphere";
}


void visSurvey::PickSetDisplay::removePick( const Geometry::Pos& pos )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	if ( marker->centerPos() == pos )
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
	    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	    {
		visBase::DataObject* dataobj =
		    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

		if ( dataobj->selectable() )
		{
		    mousepressid = eventinfo.pickedobjids[idx];
		    mousepressposition = eventinfo.pickedpos;
		}
	    }
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


void visSurvey::PickSetDisplay::updatePickSz( CallBacker* cb )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	marker->setSize( picksz );
	marker->setScale( Geometry::Pos(1, 1, 2/SPM().getAppVel()) );
    }
}


void visSurvey::PickSetDisplay::fillPar( IOPar& par, 
	TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    int grpid = group->id();
    par.set( grpstr, grpid );
    par.setYN( showallstr, showall );

    int type = getType();
    par.set( shapestr, type );

    if ( saveids.indexOf( grpid )==-1 ) saveids += grpid;
}


int visSurvey::PickSetDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int grpid;
    if ( !par.get( grpstr, grpid ) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( grpid );
    if ( !dataobj ) return 0;

    mDynamicCastGet( visBase::SceneObjectGroup*, sogrp, dataobj );
    if ( !sogrp ) return -1;

    removeChild( group->getData() );
    group->unRef();
    group = sogrp;
    group->ref();
    addChild( group->getData() );
    
    bool shwallpicks;
    if ( !par.getYN( showallstr, shwallpicks ) ) return -1;
    showAll( shwallpicks );

    int type = 0;
    par.get( shapestr, type );

    for ( int idx=0; idx<group->size(); idx++ )
    {
        mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
        if ( !marker ) continue;

	marker->setType( (visBase::Marker::Type)type );

        const float markersz = marker->getSize();
	picksz = markersz;
//	break;
    }
 
    return 1;
}
