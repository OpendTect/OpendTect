/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.35 2002-08-05 15:38:18 nanne Exp $";

#include "vissurvpickset.h"
#include "visevent.h"
#include "visdataman.h"
#include "visplanedatadisplay.h"
#include "vissurvhorizon.h"
#include "vissceneobjgroup.h"
#include "vismaterial.h"
#include "position.h"
#include "vismarker.h"
#include "viscube.h"
#include "geompos.h"
#include "color.h"
#include "iopar.h"
#include "survinfo.h"

mCreateFactoryEntry( visSurvey::PickSetDisplay );

const char* visSurvey::PickSetDisplay::nopickstr = "No Picks";
const char* visSurvey::PickSetDisplay::pickprefixstr = "Pick ";
const char* visSurvey::PickSetDisplay::showallstr = "Show all";
const char* visSurvey::PickSetDisplay::shapestr = "Shape";
const char* visSurvey::PickSetDisplay::sizestr = "Size";

visSurvey::PickSetDisplay::PickSetDisplay()
    : group( visBase::SceneObjectGroup::create() )
    , eventcatcher( visBase::EventCatcher::create() )
    , initsz(5)
    , picktype(0)
    , changed(this)
    , VisualObjectImpl(true)
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
    group->addObject( marker );

    marker->setCenterPos( pos );
    marker->setScale( Geometry::Pos(1, 1, 2/SPM().getAppVel()) );
    marker->setSize( picksz );
    marker->setType( (visBase::Marker::Type)picktype );
    marker->setMaterial( 0 );

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
    dist = SI().zRange(false).step * SPM().getAppVel() / 2;

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
    picktype = tp;
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

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    		visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	if ( dataobj->selectable() )
	{
	    eventid = eventinfo.pickedobjids[idx];
	    break;
	}
    }

    if ( eventinfo.pressed )
    {
	mousepressid = eventid;
	if ( eventid==-1 )
	{
	    mousepressposition.x = mUndefValue;
	    mousepressposition.y = mUndefValue;
	    mousepressposition.z = mUndefValue;
	}
	else
	{
	    mousepressposition = eventinfo.pickedpos;
	}

	eventcatcher->eventIsHandled();
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid )
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
		 eventid==mousepressid )
	    {
		const int sz = eventinfo.pickedobjids.size();
		bool validpicksurface = false;

		for ( int idx=0; idx<sz; idx++ )
		{
		    const DataObject* pickedobj =
			visBase::DM().getObj(eventinfo.pickedobjids[idx]);

		    if ( typeid(*pickedobj) ==
			    typeid(visSurvey::PlaneDataDisplay) ||
		          typeid(*pickedobj) ==
		      	    typeid(visSurvey::HorizonDisplay) )
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

    const int nrpicks = group->size();
    par.set( nopickstr, nrpicks );

    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const SceneObject* so = group->getObject( idx );
        mDynamicCastGet(const visBase::Marker*, marker, so );
	Geometry::Pos pos = marker->centerPos();
	BufferString key = pickprefixstr; key += idx;
	par.set( key, pos.x, pos.y, pos.z );
    }

    par.setYN( showallstr, showall );

    int type = getType();
    par.set( shapestr, type );
    par.set( sizestr, picksz );
}


int visSurvey::PickSetDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    picktype = 0;
    par.get( shapestr, picktype );

    picksz = 5;
    par.get( sizestr, picksz );

    bool shwallpicks = true;
    par.getYN( showallstr, shwallpicks );
    showAll( shwallpicks );

    group->removeAll();

    bool usedoldpar = false;
    int nopicks;
    if ( !par.get( nopickstr, nopicks ) )
    {
	res = useOldPar( par );
	if ( res < 1 ) return res;
	usedoldpar = true;
    }

    if ( usedoldpar ) return 1;
    
    for ( int idx=0; idx<nopicks; idx++ )
    {
	Geometry::Pos pos;
	BufferString key = pickprefixstr; key += idx;
	if ( !par.get( key, pos.x, pos.y, pos.z ) )
	    return -1;

	addPick( pos );
    }

    return 1;
}


int visSurvey::PickSetDisplay::useOldPar( const IOPar& par )
{
    int grpid;
    const char* grpstr = "Group";
    if ( !par.get( grpstr, grpid ) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( grpid );
    if ( !dataobj ) return 0;

    mDynamicCastGet( visBase::SceneObjectGroup*, sogrp, dataobj );
    if ( !sogrp ) return -1;

    for ( int idx=0; idx<sogrp->size(); idx++ )
    {
        mDynamicCastGet(visBase::Cube*, cube, sogrp->getObject( idx ) );
        if ( !cube ) continue;
	Geometry::Pos pos = cube->centerPos();
	addPick( pos );
    }

    return 1;
}
