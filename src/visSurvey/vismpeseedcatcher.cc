/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.9 2006-05-05 21:00:51 cvskris Exp $";

#include "vismpeseedcatcher.h"

#include "attribdataholder.h"
#include "emmanager.h"
#include "emobject.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "visseis2ddisplay.h"
#include "vismpe.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher );


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : click( this )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , clickednode_(-1,-1,-1)
{ }


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
}


void MPEClickCatcher::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = nev;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher_->ref();
    }
}


void MPEClickCatcher::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();
}


visBase::Transformation* MPEClickCatcher::getDisplayTransformation()
{ return transformation_; }


EM::PosID MPEClickCatcher::clickedNode() const
{ return clickednode_; }


bool  MPEClickCatcher::ctrlClicked() const     
{ return ctrlclicked_; }


bool MPEClickCatcher::shiftClicked() const    
{ return shiftclicked_; }


const Coord3& MPEClickCatcher::clickedPos() const
{ return clickedpos_; }

int  MPEClickCatcher::clickedObjectID() const
{ return clickedobjid_;}


const CubeSampling& MPEClickCatcher::clickedObjectCS() const
{ return clickedcs_; }


const Attrib::DataCubes* MPEClickCatcher::clickedObjectData() const
{ return attrdata_; }


const Attrib::SelSpec* MPEClickCatcher::clickedObjectDataSelSpec() const
{ return as_; }


void MPEClickCatcher::clickCB( CallBacker* cb )
{
    if ( eventcatcher_->isEventHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return;

    if ( eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return;

    if ( !eventinfo.alt && (!eventinfo.ctrl || !eventinfo.shift) )
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    if ( !dataobj ) 
		continue;

	    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	    if ( emod )
	    {
		sendPlanesContainingNode( visid, emod, eventinfo );
		eventcatcher_->eventIsHandled();
		break;
	    }

	    if ( eventinfo.ctrl || eventinfo.shift )
		continue;
	    
	    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	    if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
	    {
		sendClickEvent( EM::PosID(-1,-1,-1), eventinfo.ctrl, 
				eventinfo.shift, eventinfo.pickedpos, 
				visid, plane->getCubeSampling(),
				plane->getCacheVolume(false),
				plane->getSelSpec(0) );
		eventcatcher_->eventIsHandled();
		break;
	    }

	    mDynamicCastGet( MPEDisplay*, mpedisplay, dataobj );
	    if ( mpedisplay && mpedisplay->isDraggerShown() &&
		 !mpedisplay->isManipulatorShown() &&
		 mpedisplay->getPlanePosition(clickedcs_) ) 
	    {
		sendClickEvent( EM::PosID(-1,-1,-1), eventinfo.ctrl, 
				eventinfo.shift, eventinfo.pickedpos, 
				visid, clickedcs_, 0, 0 );
		eventcatcher_->eventIsHandled();
		break;
	    }

	    mDynamicCastGet( Seis2DDisplay*, seis2ddisplay, dataobj );
	    if ( seis2ddisplay )
	    {
		RefMan<const Attrib::Data2DHolder> cache =
						seis2ddisplay->getCache();
		RefMan<Attrib::DataCubes> cube = 0;

		if ( cache )
		{
 		    cube = new Attrib::DataCubes;
		    if ( !cache->fillDataCube(*cube) )
			cube = 0;
		}

		sendClickEvent( EM::PosID(-1,-1,-1), eventinfo.ctrl, 
				eventinfo.shift, eventinfo.pickedpos, 
				visid, clickedcs_, cube,
				seis2ddisplay->getSelSpec(0),
		      		cache, seis2ddisplay->name(),
				&seis2ddisplay->lineSetID() );
		eventcatcher_->eventIsHandled();
		break;
	    }
	}
    }
}


void MPEClickCatcher::sendPlanesContainingNode( 
			    int visid, const visSurvey::EMObjectDisplay* emod,
			    const visBase::EventInfo& eventinfo )
{
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode,
						       eventinfo.pickedobjids );
    if ( nodepid.objectID()==-1 ) 
	return;

    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;

    const Coord3 nodepos = emobj->getPos( nodepid );
    const BinID nodebid = SI().transform( nodepos );
    
    TypeSet<int> planesinscene;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), planesinscene ); 
    
    for ( int idx=0; idx<planesinscene.size(); idx++ )
    {
	visBase::DataObject* planeobj = 
				visBase::DM().getObject( planesinscene[idx] );
	if ( !planeobj )
	    continue;

	mDynamicCastGet( PlaneDataDisplay*, plane, planeobj );
	if ( !plane || plane->getOrientation()==PlaneDataDisplay::Timeslice ) 
	    continue;

	const CubeSampling cs = plane->getCubeSampling();
	if ( cs.hrg.includes(nodebid) && cs.zrg.includes(nodepos.z) )
	{
	    sendClickEvent( nodepid, eventinfo.ctrl, eventinfo.shift,
			    eventinfo.pickedpos, visid, cs, 
			    plane->getCacheVolume(false), 
			    plane->getSelSpec(0) );
	}
    }
}


void MPEClickCatcher::sendClickEvent( const EM::PosID pid, bool ctrl, 
				      bool shift, const Coord3& coord, 
				      int visid, const CubeSampling& cs,
       				      const Attrib::DataCubes* ss,
				      const Attrib::SelSpec* selspec,
				      const Attrib::Data2DHolder* linedata,
				      const char* linename,
				      const MultiID* lineset)
{
    clickednode_ = pid;
    ctrlclicked_ = ctrl;
    shiftclicked_ = shift;
    clickedcs_ = cs;
    clickedobjid_ = visid;
    clickedpos_ = coord;
    as_ = selspec;
    attrdata_ = ss;
    linedata_ = linedata;
    if ( lineset ) lineset_ = *lineset;
    linename_ = linename;


    click.trigger();

    clickednode_ = EM::PosID(-1,-1,-1);
    ctrlclicked_ = false;
    shiftclicked_ = false;
    clickedobjid_ = -1;
    as_ = 0;
    attrdata_ = 0;
    clickedcs_.init(false);
    clickedpos_ = Coord3::udf();
    linedata_ = 0;
    lineset_ = -1;
    linename_ = "";

}

}; //namespce
