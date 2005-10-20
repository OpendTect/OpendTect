/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: mpeengine.cc,v 1.49 2005-10-20 20:01:20 cvskris Exp $";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribslice.h"
#include "bufstringset.h"
#include "emeditor.h"
#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "executor.h"
#include "geomelement.h"
#include "iopar.h"
#include "sectiontracker.h"
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
    : trackplanechange( this )
    , activevolumechange( this )
    , loadEMObject( this )
{
    trackers.allowNull(true);
    init();
}


Engine::~Engine()
{
    deepErase( trackers );
    deepErase( editors );
    deepUnRef( attribcache );
    deepErase( attribcachespecs );
    deepErase( trackerfactories );
    deepErase( editorfactories );
}


const CubeSampling& Engine::activeVolume() const
{ return activevolume; }


void Engine::setActiveVolume( const CubeSampling& nav )
{
    activevolume = nav;

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

    ntp.setTrackMode( trackPlane().getTrackMode() );
    setTrackPlane( ntp, false );
    activevolumechange.trigger();
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


Executor* Engine::trackInVolume()
{
    ExecutorGroup* res = 0;
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( !trackers[idx] || !trackers[idx]->isEnabled() )
	    continue;

	if ( !res ) res = new ExecutorGroup("Autotrack", false );

	res->add( trackers[idx]->trackInVolume() );
    }

    return res;
}

void Engine::setTrackMode( TrackPlane::TrackMode tm )
{
    trackplane.setTrackMode( tm );
    trackplanechange.trigger();
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


void Engine::removeTracker( int idx )
{
    if ( idx<0 || idx>=trackers.size() )
	return;

    delete trackers[idx];
    trackers.replace( idx, 0 );

   
    for ( int idy=0; idy<trackers.size(); idx++ )
    {
	if ( trackers[idx] )
	    return;
    }

    setActiveVolume( getDefaultActiveVolume() );
}


int Engine::highestTrackerID() const
{ return trackers.size()-1; }


const EMTracker* Engine::getTracker( int idx ) const
{ return const_cast<Engine*>(this)->getTracker(idx); }


EMTracker* Engine::getTracker( int idx ) 
{ return idx<0 || idx<trackers.size() ? trackers[idx] : 0; }


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


void Engine::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers.size(); trackeridx++ )
    {
	EMTracker* tracker = trackers[trackeridx];
	if ( !tracker ) continue;

	ObjectSet<const Attrib::SelSpec> specs;
	tracker->getNeededAttribs(specs);
	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec* as = specs[idx];
	    if ( indexOf(res,*as) < 0 )
		res += as;
	}
    }
}


CubeSampling Engine::getAttribCube( const Attrib::SelSpec& as ) const
{
    CubeSampling res( engine().activeVolume() );
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	const CubeSampling cs = trackers[idx]->getAttribCube( as );
	res.include(cs);
    }

    return res;
}
    

const Attrib::SliceSet* Engine::getAttribCache(const Attrib::SelSpec& as) const
{
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==as )
	    return idx<attribcache.size() ? attribcache[idx] : 0;
    }

    return 0;
}


bool Engine::setAttribData( const Attrib::SelSpec& as, 
			    const Attrib::SliceSet* newdata )
{
    bool found = false;
    for ( int idx=0; idx<attribcachespecs.size(); idx++ )
    {
	if ( *attribcachespecs[idx]==as )
	{
	    attribcache[idx]->unRef();
	    attribcache.replace(idx, newdata);
	    found = true;
	    break;
	}
    }

    if ( !found )
    {
	attribcachespecs += new Attrib::SelSpec(as);
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


void Engine::fillPar( IOPar& iopar ) const
{
    int trackeridx = 0;
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	EMTracker* tracker = trackers[idx];
	if ( !tracker ) continue;
	
	IOPar localpar;
	localpar.set( sKeyObjectID(),EM::EMM().getMultiID(tracker->objectID()));
	localpar.setYN( sKeyEnabled(), tracker->isEnabled() );

	tracker->fillPar( localpar );

	BufferString key( trackeridx );
	iopar.mergeComp( localpar, key );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume.fillPar( iopar );

    IOPar tppar;
    trackplane.fillPar( tppar );
    iopar.mergeComp( tppar, sKeyTrackPlane() );
}


bool Engine::usePar( const IOPar& iopar )
{
    init();

    CubeSampling newvolume;
    if ( newvolume.usePar(iopar) )
    {
	setActiveVolume( newvolume );
	PtrMan<IOPar> tppar = iopar.subselect( sKeyTrackPlane() );
	if ( tppar ) trackplane.usePar( *tppar );
    }

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    iopar.get( sKeyNrTrackers(), nrtrackers );

    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	BufferString key( idx );
	PtrMan<IOPar> localpar = iopar.subselect( key );
	if ( !localpar ) continue;
	
	if ( !localpar->get(sKeyObjectID(),midtoload) ) continue;
	EM::ObjectID oid = EM::EMM().getObjectID( midtoload );
	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj )
	{
	    loadEMObject.trigger();
	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	}

	if ( !emobj )
	{
	    PtrMan<Executor> exec =
		EM::EMM().objectLoader( MPE::engine().midtoload );
	    if ( exec ) exec->execute();

	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	}

	if ( !emobj )
	    continue;
	    
	const int trackeridx = addTracker( emobj );
	if ( trackeridx < 0 ) continue;
	EMTracker* tracker = trackers[trackeridx];
	
	bool doenable = true;
	localpar->getYN( sKeyEnabled(), doenable );
	tracker->enable( doenable );

	tracker->usePar( *localpar );
    }

    return true;
}


void Engine::init()
{
    deepErase( trackers );
    deepErase( editors );
    deepUnRef( attribcache );
    deepErase( attribcachespecs );
    setActiveVolume( getDefaultActiveVolume() );
}



}; // namespace MPE


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
