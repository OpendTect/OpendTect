/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: mpeengine.cc,v 1.26 2005-05-03 11:38:35 cvsnanne Exp $";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribslice.h"
#include "bufstringset.h"
#include "emeditor.h"
#include "sectiontracker.h"
#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "geomelement.h"
#include "survinfo.h"


#define mRetErr( msg, retval ) { errmsg = msg; return retval; }


MPE::Engine& MPE::engine()
{
    static MPE::Engine* theinst_ = 0;
    if ( !theinst_ )
	theinst_ = new MPE::Engine;
    return *theinst_;
}


namespace MPE 
{

Engine::Engine()
    : seedpropertychange( this )
    , trackplanechange( this )
    , activevolumechange( this )
    , seedsize( 4 )
    , seedlinewidth( 3 )
    , seedcolor( Color::White )
{
    setActiveVolume( getDefaultActiveVolume() );
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


const CubeSampling& Engine::activeVolume() const
{ return activevolume; }


void Engine::setActiveVolume( const CubeSampling& nav )
{
    activevolume = nav;
    activevolumechange.trigger();

    int dim = 0;
    if ( trackplane.boundingBox().hrg.start.crl==
	 trackplane.boundingBox().hrg.stop.crl )
	dim = 1;
    else if ( !trackplane.boundingBox().zrg.width() )
	dim = 2;

    TrackPlane ntp;
    CubeSampling& ncs = ntp.boundingBox();
    ncs = nav;

    if ( !dim )
	ncs.hrg.start.inl = ncs.hrg.stop.inl =
	    SI().inlRange(true).snap((ncs.hrg.start.inl+ncs.hrg.stop.inl)/2);
    else if ( dim==1 )
	ncs.hrg.start.crl = ncs.hrg.stop.crl =
	    SI().crlRange(true).snap((ncs.hrg.start.crl+ncs.hrg.stop.crl)/2);
    else
	ncs.zrg.start = ncs.zrg.stop = SI().zRange(true).snap(ncs.zrg.center());

    setTrackPlane( ntp, false );
}


const TrackPlane& Engine::trackPlane() const
{ return trackplane; }


bool Engine::setTrackPlane( const TrackPlane& ntp, bool dotrack )
{
    trackplane = ntp;
    trackplanechange.trigger();
    return dotrack ? trackAtCurrentPlane() : true;
}


bool Engine::trackAtCurrentPlane()
{
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx] || !trackers[idx]->isEnabled() )
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
	if ( !trackers[idx] || !trackers[idx]->isEnabled() )
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


void Engine::setTrackMode( TrackPlane::TrackMode tm )
{
    trackplane.setTrackMode( tm );
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

    if ( getTrackerByObject(obj->id()) != -1 )
	mRetErr( "Object is already tracked", -1 );

    bool added = false;
    for ( int idx=0; idx<trackerfactories.size(); idx++ )
    {
	if ( !strcmp(obj->getTypeStr(),trackerfactories[idx]->emObjectType()) )
	{
	    EMTracker* tracker = trackerfactories[idx]->create(obj);
	    trackers += tracker;
	    added = true;
	    break;
	}
    }

    if ( !added )
	mRetErr( "Cannot find this trackertype", -1 );

    return trackers.size()-1;
}


int Engine::addTracker( const char* objname, const char* trackername )
{
    if ( !interactionseeds.size() || !objname || !trackername )
	mRetErr( "No seeds or no name specified", -1 );

    if ( getTrackerByObject(objname)!=-1 )
	mRetErr( "Object with this name does already exist", -1 );

    bool added = false;
    for ( int idx=0; idx<trackerfactories.size(); idx++ )
    {
	if ( !strcmp(trackername,trackerfactories[idx]->emObjectType()) )
	{
	    EMTracker* tracker = trackerfactories[idx]->create();
	    if ( !tracker->setSeeds(interactionseeds,objname) )
	    {
		errmsg = tracker->errMsg();
		delete tracker;
		interactionseeds.erase();
		return -1;
	    }

	    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
	    emobj->setPreferredColor( seedcolor );
	    trackers += tracker;
	    added = true;
	    break;
	}
    }

    if ( !added )
	mRetErr( "Cannot find this trackertype", -1 );

    return trackers.size()-1;
}


void Engine::removeTracker( int idx )
{
    if ( idx>=trackers.size() )
	return;

    delete trackers[idx];
    trackers.replace( 0, idx );
}


int Engine::highestTrackerID() const
{ return trackers.size(); }


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


void Engine::getNeededAttribs( ObjectSet<const AttribSelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers.size(); trackeridx++ )
    {
	EMTracker* tracker = trackers[trackeridx];
	if ( !tracker ) continue;

	EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
	if ( !emobj ) continue;

	for ( int sectidx=0; sectidx<emobj->nrSections(); sectidx++ )
	{
	    const EM::SectionID sectionid = emobj->sectionID( sectidx );
	    SectionTracker* sectiontracker = 
				tracker->getSectionTracker( sectionid );
	    if ( !sectiontracker ) continue;

	    ObjectSet<const AttribSelSpec> specs;
	    sectiontracker->getNeededAttribs( specs );
	    for ( int idx=0; idx<specs.size(); idx++ )
	    {
		const AttribSelSpec* as = specs[idx];
		if ( indexOf(res,*as) < 0 )
		    res += as;
	    }
	}
    }
}


CubeSampling Engine::getAttribCube( const AttribSelSpec& as ) const
{
    const AttribSliceSet* sliceset = getAttribCache( as );
    return sliceset ? sliceset->sampling : activeVolume();
}
    

const AttribSliceSet* Engine::getAttribCache( const AttribSelSpec& as ) const
{
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==as )
	    return idx<attribcache.size() ? attribcache[idx] : 0;
    }

    return 0;
}


bool Engine::setAttribData( const AttribSelSpec& as, AttribSliceSet* newdata )
{
    bool found = false;
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==as )
	{
	    delete attribcache[idx];
	    attribcache.replace(newdata,idx);
	    found = true;
	    break;
	}
    }

    if ( !found )
    {
	attribcachespecs += new AttribSelSpec(as);
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

	ObjectEditor* editor = editorfactories[idx]->create(*emobj);
	editors += editor;
	return editor;
    }

    return 0;
}


void Engine::removeEditor( const EM::ObjectID& objid )
{
    ObjectEditor* editor = getEditor( objid, false );
    if ( editor )
    {
	editors -= editor;
	delete editor;
    }
}


const char* Engine::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0 ; }


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


CubeSampling Engine::getDefaultActiveVolume()
{
    CubeSampling cs;
    cs.hrg.start.inl=(5*SI().inlRange(true).start+3*SI().inlRange(true).stop)/8;
    cs.hrg.start.crl=(5*SI().crlRange(true).start+3*SI().crlRange(true).stop)/8;
    cs.hrg.stop.inl =(3*SI().inlRange(true).start+5*SI().inlRange(true).stop)/8;
    cs.hrg.stop.crl =(3*SI().crlRange(true).start+5*SI().crlRange(true).stop)/8;
    cs.zrg.start = (5*SI().zRange(true).start+3*SI().zRange(true).stop)/ 8;
    cs.zrg.stop = (3*SI().zRange(true).start+5*SI().zRange(true).stop)/8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;
    return cs;
}


}; //namespace

#include "emhortubeeditor.h"
#include "emhortubetracker.h"
#include "emfaulteditor.h"
#include "emfaulttracker.h"
#include "emhorizoneditor.h"
#include "emhorizontracker.h"


void MPE::initStandardClasses()
{
#ifdef __debug__
    MPE::HorizontalTubeEditor::initClass();
    MPE::HorizontalTubeTracker::initClass();
#endif
    MPE::FaultEditor::initClass();
    MPE::FaultTracker::initClass();
    MPE::HorizonEditor::initClass();
    MPE::HorizonTracker::initClass();
}
