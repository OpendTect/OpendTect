/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: mpeengine.cc,v 1.2 2005-01-07 12:18:29 kristofer Exp $";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribslice.h"
#include "bufstringset.h"
#include "emeditor.h"
#include "emlens.h"
#include "emlenstracker.h"
#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "geomelement.h"


#define mRetErr( msg, retval ) { errmsg = msg; return retval; }

namespace MPE 
{


Engine::Engine()
    : seedpropertychange( this )
{
    trackers.allowNull(true);
}


Engine::~Engine()
{
    deepErase( interactionseeds );
    deepErase( trackers );
    deepErase( editors );
    deepErase( attribcache );
    deepErase( attribcachespecs );
    deepErase( trackerfactories );
    deepErase( editorfactories );
}


const CubeSampling& Engine::activeVolume() const { return activevolume; }


void  Engine::setActiveVolume(const CubeSampling& nav) { activevolume = nav; }


const TrackPlane& Engine::trackPlane() const { return trackplane; }


bool  Engine::setTrackPlane(const TrackPlane& ntp, bool dotrack)
{
    trackplane = ntp;
    return dotrack ? trackAtCurrentPlane() : true;
}


bool Engine::trackAtCurrentPlane()
{
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx]->isEnabled() )
	    continue;

	if ( !trackers[idx]->trackSections( trackplane ) )
	{
	    errmsg = "Error while tracking ";
	    errmsg += trackers[idx]->objectName();
	    errmsg += ": ";
	    errmsg += trackers[idx]->errMsg();
	    return false;
	}
    }

    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx]->isEnabled() )
	    continue;

	if ( !trackers[idx]->trackIntersections( trackplane ) )
	{
	    errmsg = "Error while tracking ";
	    errmsg += trackers[idx]->objectName();
	    errmsg += ": ";
	    errmsg += trackers[idx]->errMsg();
	    return false;
	}
    }

    return true;
}


void Engine::getAvaliableTrackerTypes( BufferStringSet& res ) const
{
    res.deepErase();
    for ( int idx=0; idx<trackerfactories.size(); idx++ )
	res.add(trackerfactories[idx]->emObjectType());
}


int Engine::addTracker( EM::EMObject* obj )
{
    if ( !obj )
	mRetErr( "No valid object", -1 );

    if ( getTrackerByObject(obj->id()) )
	mRetErr( "Object is allready tracked", -1 );

    for ( int idx=0; idx<trackerfactories.size(); idx++ )
    {
	if ( !strcmp( obj->getTypeStr(),
		      trackerfactories[idx]->emObjectType()) )
	{
	    EMTracker* tracker = trackerfactories[idx]->create(obj);
	    trackers += tracker;
	    break;
	}
    }

    return trackers.size()-1;
}


int Engine::addTracker( const char* objname, const char* trackername )
{
    if ( !interactionseeds.size() || !objname || !trackername )
	mRetErr( "No seeds or no name specified", -1 );

    if ( getTrackerByObject(objname) )
	mRetErr( "Object with this name does already exist", -1 );

    for ( int idx=0; idx<trackerfactories.size(); idx++ )
    {
	if ( !strcmp( trackername, trackerfactories[idx]->emObjectType()) )
	{
	    EMTracker* tracker = trackerfactories[idx]->create();
	    if ( !tracker->setSeeds( interactionseeds, objname ) )
	    {
		errmsg = tracker->errMsg();
		delete tracker;
		return -1;
	    }

	    trackers += tracker;
	    break;
	}
    }

    return trackers.size()-1;
}


void Engine::removeTracker(int idx)
{
    if ( idx>=trackers.size() )
	return;

    delete trackers[idx];
    trackers.replace( 0, idx );
}


int Engine::highestTrackerID() const { return trackers.size(); }


const EMTracker* Engine::getTracker( int idx ) const
{ return const_cast<Engine*>(this)->getTracker(idx); }


EMTracker* Engine::getTracker( int idx ) 
{ return idx<trackers.size() ? trackers[idx] : 0; }


int Engine::getTrackerByObject( const EM::ObjectID& oid ) const
{
    if ( oid==-1 ) return -1;

    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx] ) continue;

	if ( oid==trackers[idx]->objectID() )
	    return idx;
    }

    return -1;
}


int Engine::getTrackerByObject( const char* objname ) const
{
    if ( !objname || !objname[0] ) return -1;

    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx] ) continue;

	if ( !strcmp(objname,trackers[idx]->objectName()) )
	    return idx;
    }

    return -1;
}


void Engine::getNeededAttribs(ObjectSet<const AttribSelSpec>& res) const
{
    //TODO
}


CubeSampling Engine::getAttribCube(const AttribSelSpec&) const
{
    //TODO: Implement margins
    return activeVolume();
}
    

const AttribSliceSet* Engine::getAttribCache( const AttribSelSpec& spec ) const
{
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==spec ) return attribcache[idx];
    }

    return 0;
}


bool Engine::setAttribData(const AttribSelSpec& spec, AttribSliceSet* newdata )
{
    bool found = false;
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==spec )
	{
	    delete attribcache[idx];
	    attribcache.replace(newdata,idx);
	    found = true;
	    break;
	}
    }

    if ( !found )
    {
	attribcachespecs += new AttribSelSpec(spec);
	attribcache += newdata;
    }

    //Todo - give to trackers
    return true;
}


ObjectEditor* Engine::getEditor( const EM::ObjectID& id, bool create )
{
    for ( int idx=0; idx<editors.size(); idx++ )
    {
	if ( editors[idx]->emObject().id()==id )
	    return editors[idx];
    }

    if ( !create ) return 0;

    EM::EMObject* emobj = EM::EMM().getObject(id);
    if ( !emobj ) return 0;

    for ( int idx=0; idx<editorfactories.size(); idx++ )
    {
	if ( strcmp(editorfactories[idx]->emObjectType(), emobj->getTypeStr()) )
	    continue;

	ObjectEditor* editor = editorfactories[idx]->create(emobj);
	editors += editor;
	return editor;
    }

    return 0;
}


const char* Engine::errMsg() const { return errmsg[0] ? errmsg : 0 ; }


void Engine::addTrackerFactory( TrackerFactory* ntf )
{
    for ( int idx=0; idx<trackerfactories.size(); idx++ )
    {
	if ( !strcmp(ntf->emObjectType(),trackerfactories[idx]->emObjectType()))
	{
	    delete ntf;
	    return;
	}
    }

    trackerfactories += ntf;
}


void Engine::addEditorFactory( EditorFactory* nef )
{
    for ( int idx=0; idx<editorfactories.size(); idx++ )
    {
	if ( !strcmp(nef->emObjectType(),editorfactories[idx]->emObjectType()))
	{
	    delete nef;
	    return;
	}
    }

    editorfactories += nef;
}


};  //namespace
