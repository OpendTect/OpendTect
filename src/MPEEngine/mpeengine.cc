/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "emeditor.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurface.h"
#include "emtracker.h"
#include "executor.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "sectiontracker.h"
#include "survinfo.h"


#define mRetErr( msg, retval ) { errmsg_ = msg; return retval; }


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
    , trackplanetrack( this )
    , activevolumechange( this )
    , trackeraddremove( this )
    , loadEMObject( this )
    , oneactivetracker_( 0 )
    , activetracker_( 0 )
    , isactivevolshown_(false)
    , activefaultid_( -1 )
    , activefssid_( -1 )
    , activefaultchanged_( this )
    , activefsschanged_( this )
{
    trackers_.allowNull(true);
    flatcubescontainer_.allowNull(true);
    init();
}


Engine::~Engine()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
}


const CubeSampling& Engine::activeVolume() const
{ return activevolume_; }


void Engine::setActiveVolume( const CubeSampling& nav )
{
    activevolume_ = nav;

    int dim = 0;
    if ( trackplane_.boundingBox().hrg.start.crl==
	 trackplane_.boundingBox().hrg.stop.crl )
	dim = 1;
    else if ( !trackplane_.boundingBox().zrg.width() )
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


void Engine::setActive2DLine( const MultiID& linesetid, const char* linename )
{
    active2dlinesetid_ = linesetid;
    active2dlinename_ = linename;
}


const MultiID& Engine::active2DLineSetID() const
{ return active2dlinesetid_; }


const BufferString& Engine::active2DLineName() const
{ return active2dlinename_; }


const TrackPlane& Engine::trackPlane() const
{ return trackplane_; }


bool Engine::setTrackPlane( const TrackPlane& ntp, bool dotrack )
{
    trackplane_ = ntp;
    trackplanechange.trigger();
    if ( ntp.getTrackMode()==TrackPlane::Move ||
	    ntp.getTrackMode()==TrackPlane::None )
	dotrack = false;

    return dotrack ? trackAtCurrentPlane() : true;
}


#define mTrackAtCurrentPlane(func) \
    for ( int idx=0; idx<trackers_.size(); idx++ ) \
    { \
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() ) \
	    continue; \
	EM::ObjectID oid = trackers_[idx]->objectID(); \
	if ( EM::EMM().getObject(oid)->isLocked() ) \
	    continue; \
	if ( !trackers_[idx]->func( trackplane_ ) ) \
	{ \
	    errmsg_ = "Error while tracking "; \
	    errmsg_ += trackers_[idx]->objectName(); \
	    errmsg_ += ": "; \
	    errmsg_ += trackers_[idx]->errMsg(); \
	    return false; \
	} \
    } 

bool Engine::trackAtCurrentPlane()
{
    trackplanetrack.trigger();
    mTrackAtCurrentPlane(trackSections);
    mTrackAtCurrentPlane(trackIntersections);
    return true;
}


void Engine::updateSeedOnlyPropagation( bool yn )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	EM::EMObject* emobj = EM::EMM().getObject( oid );

	for ( int stidx=0; stidx<emobj->nrSections(); stidx++ )
	{
	    SectionTracker* sectiontracker = 
			trackers_[idx]->getSectionTracker( stidx, true );
	    sectiontracker->setSeedOnlyPropagation( yn );
	}
    }
}


Executor* Engine::trackInVolume()
{
    ExecutorGroup* res = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	if ( EM::EMM().getObject(oid)->isLocked() )
	    continue;

	if ( !res ) res = new ExecutorGroup("Autotrack", false );

	res->add( trackers_[idx]->trackInVolume() );
    }

    return res;
}


void Engine::removeSelectionInPolygon( const Selector<Coord3>& selector,
       				       TaskRunner* tr )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	EM::EMM().removeSelected( oid, selector, tr );

	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj->getRemovedPolySelectedPosBox().isEmpty() )
	{
	    int dim = 0;
	    if ( trackplane_.boundingBox().hrg.start.crl==
		 trackplane_.boundingBox().hrg.stop.crl )
		dim = 1;
	    else if ( !trackplane_.boundingBox().zrg.width() )
		dim = 2;

	    TrackPlane ntp;
	    CubeSampling& ncs = ntp.boundingBox();
	    ncs = emobj->getRemovedPolySelectedPosBox();

	    if ( !dim )
		ncs.hrg.start.inl = ncs.hrg.stop.inl =
		    SI().inlRange(true).snap(
			    (ncs.hrg.start.inl+ncs.hrg.stop.inl)/2);
	    else if ( dim==1 )
		ncs.hrg.start.crl = ncs.hrg.stop.crl =
		    SI().crlRange(true).snap(
			    (ncs.hrg.start.crl+ncs.hrg.stop.crl)/2);
	    else
		ncs.zrg.start = ncs.zrg.stop = SI().zRange(true).snap(
							ncs.zrg.center());
	    ntp.setTrackMode( trackPlane().getTrackMode() );
	    setTrackPlane( ntp, false );
	    //setActiveVolume( emobj->getRemovedPolySelectedPosBox() );
	    emobj->emptyRemovedPolySelectedPosBox();
	}
    }
}


void Engine::setTrackMode( TrackPlane::TrackMode tm )
{
    trackplane_.setTrackMode( tm );
    trackplanechange.trigger();
}


void Engine::getAvailableTrackerTypes( BufferStringSet& res ) const
{ res = TrackerFactory().getNames(); }


int Engine::addTracker( EM::EMObject* obj )
{
    if ( !obj )
	mRetErr( "No valid object", -1 );

    const int idx = getTrackerByObject( obj->id() );

    if ( idx != -1 )
    {
	trackers_[idx]->ref();
	return idx;
    }
	//mRetErr( "Object is already tracked", -1 );

    EMTracker* tracker = TrackerFactory().create( obj->getTypeStr(), obj );
    if ( !tracker )
	mRetErr( "Cannot find this trackertype", -1 );

    tracker->ref();
    trackers_ += tracker;
    ObjectSet<FlatCubeInfo>* flatcubes = new ObjectSet<FlatCubeInfo>;
    flatcubescontainer_ += flatcubes;
    trackeraddremove.trigger();

    return trackers_.size()-1;
}


void Engine::removeTracker( int idx )
{
    if ( idx<0 || idx>=trackers_.size() || !trackers_[idx] )
	return;

    const int noofref = trackers_[idx]->nrRefs();
    trackers_[idx]->unRef();

    if ( noofref != 1 )
	return;

    trackers_.replace( idx, 0 );

    deepErase( *flatcubescontainer_[idx] );
    flatcubescontainer_.replace( idx, 0 );

    trackeraddremove.trigger();
}


int Engine::nrTrackersAlive() const
{
    int nrtrackers = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( trackers_[idx] )
	    nrtrackers++;
    }
    return nrtrackers;
}
	

int Engine::highestTrackerID() const
{ return trackers_.size()-1; }


const EMTracker* Engine::getTracker( int idx ) const
{ return const_cast<Engine*>(this)->getTracker(idx); }


EMTracker* Engine::getTracker( int idx ) 
{ return idx>=0 && idx<trackers_.size() ? trackers_[idx] : 0; }


int Engine::getTrackerByObject( const EM::ObjectID& oid ) const
{
    if ( oid==-1 ) return -1;

    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;

	if ( oid==trackers_[idx]->objectID() )
	    return idx;
    }

    return -1;
}


int Engine::getTrackerByObject( const char* objname ) const
{
    if ( !objname || !objname[0] ) return -1;

    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;

	if ( !strcmp(objname,trackers_[idx]->objectName()) )
	    return idx;
    }

    return -1;
}


void Engine::setActiveTracker( EMTracker* tracker )
{ activetracker_ = tracker; }


void Engine::setActiveTracker( const EM::ObjectID& objid )
{
    const int tridx = getTrackerByObject( objid );
    activetracker_ = trackers_.validIdx(tridx) ? trackers_[tridx] : 0;
}


EMTracker* Engine::getActiveTracker()
{ return activetracker_; }


void Engine::setOneActiveTracker( const EMTracker* tracker )
{ oneactivetracker_ = tracker; }


void Engine::unsetOneActiveTracker()
{ oneactivetracker_ = 0; }


void Engine::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers_.size(); trackeridx++ )
    {
	const EMTracker* tracker = trackers_[trackeridx];
	if ( !tracker ) continue;

	if ( oneactivetracker_ && oneactivetracker_!=tracker )
	    continue;

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
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;
	const CubeSampling cs = trackers_[idx]->getAttribCube( as );
	res.include(cs);
    }

    return res;
}


bool Engine::isSelSpecSame( const Attrib::SelSpec& setupss,
			    const Attrib::SelSpec& clickedss ) const
{
    bool setupssisstoed = matchString( Attrib::StorageProvider::attribName(),
	    			       setupss.defString() );
    return setupss.id().asInt()==clickedss.id().asInt() &&
	setupssisstoed==clickedss.id().isStored() &&
	setupss.isNLA()==clickedss.isNLA() &&
	BufferString(setupss.objectRef())==BufferString(clickedss.objectRef())
	&& BufferString(setupss.defString())==
	   BufferString(clickedss.defString()) &&
	setupss.is2D()==clickedss.is2D();
}


int Engine::getCacheIndexOf( const Attrib::SelSpec& as ) const
{
    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	bool asisstored = matchString( Attrib::StorageProvider::attribName(),
				       as.defString() );

	if ( attribcachespecs_[idx]->attrsel_.is2D()	   != as.is2D()  ||
	     attribcachespecs_[idx]->attrsel_.isNLA()	   != as.isNLA() ||
	     attribcachespecs_[idx]->attrsel_.id().asInt() != as.id().asInt() ||
      	     attribcachespecs_[idx]->attrsel_.id().isStored() != asisstored )
	    continue;

	if ( !as.is2D() )
	    return idx;
	
	if ( attribcachespecs_[idx]->linesetid_ != active2DLineSetID() ||
	     attribcachespecs_[idx]->linename_  != active2DLineName()   )
	    continue;

	return idx;
    }

    return -1;
}


DataPack::ID Engine::getAttribCacheID( const Attrib::SelSpec& as ) const
{
    const int idx = getCacheIndexOf(as);
    return attribcachedatapackids_.validIdx(idx)
	? attribcachedatapackids_[idx] : DataPack::cNoID();
}


const DataHolder* Engine::getAttribCache( DataPack::ID datapackid )
{
    DataPack* datapack = DPM( DataPackMgr::FlatID() ).obtain( datapackid, true);
    if ( !datapack )
	datapack = DPM( DataPackMgr::CubeID() ).obtain( datapackid, true );

    DataHolder* dh = new DataHolder();

    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    if ( cdp )
    {
	dh->setCubeSampling( cdp->cube().cubeSampling() );
	dh->set3DData( &cdp->cube() );
    }
    mDynamicCastGet(const Attrib::Flat3DDataPack*,fdp,datapack);
    if ( fdp )
    {
	dh->setCubeSampling( fdp->cube().cubeSampling() );
	dh->set3DData( &fdp->cube() );
    }

    mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2d,datapack);
    if ( dp2d )
    {
	dh->setCubeSampling( dp2d->dataarray()->cubesampling_ );
	dh->set2DData( dp2d->dataarray() );
    }

    if ( dh->getCubeSampling().isEmpty() )
    { delete dh; return 0; }
    else
	return dh;
}


const DataHolder* Engine::getAttribCache(const Attrib::SelSpec& as)
{
    const int idx = getCacheIndexOf(as);
    return idx>=0 && idx<attribcache_.size() ? attribcache_[idx] : 0;
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    DataPack::ID cacheid )
{
    const int idx = getCacheIndexOf(as);
    if ( idx>=0 && idx<attribcachedatapackids_.size() )
    {
	if ( cacheid <= DataPack::cNoID() )
	{
	    attribcache_[idx]->unRef();
	    attribcachedatapackids_.remove( idx );
	    attribcache_.remove( idx );
	    delete attribcachespecs_.remove( idx );
	}
	else
	{
	    const DataHolder* newdata = getAttribCache( cacheid );
	    if ( newdata )
	    {
		attribcache_[idx]->unRef();
		attribcachedatapackids_[idx] = cacheid;
		attribcache_.replace( idx, newdata );
		newdata->ref();
	    }
	}
    }
    else if ( cacheid > DataPack::cNoID() )
    {
	const DataHolder* newdata = getAttribCache( cacheid );
	if ( newdata )
	{
	    attribcachespecs_ += as.is2D() ?
		new CacheSpecs( as, active2DLineSetID(), active2DLineName() ) :
		new CacheSpecs( as ) ;

	    attribcachedatapackids_ += cacheid;
	    attribcache_ += newdata;
	    newdata->ref();
	}
    }

    return true;
}


bool Engine::setAttribData( const Attrib::SelSpec& as, 
			    const DataHolder* newdata )
{
    const int idx = getCacheIndexOf(as);
    if ( idx>=0 && idx<attribcache_.size() )
    {
	attribcache_[idx]->unRef();
	if ( !newdata )
	{
	    attribcache_.remove( idx );
	    delete attribcachespecs_.remove( idx );
	}
	else
	{
	    attribcache_.replace( idx, newdata );
	    newdata->ref();
	}
    }
    else if (newdata)
    {
	attribcachespecs_ += as.is2D() ? 
	    new CacheSpecs( as, active2DLineSetID(), active2DLineName() ) :
	    new CacheSpecs( as ) ;

	attribcache_ += newdata;
	newdata->ref();
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as, 
			    const CubeSampling& cs )
{
    const DataHolder* cache = getAttribCache( as );
    if ( !cache ) 
	return false;

    CubeSampling cachedcs = cache->getCubeSampling();
    const float zrgeps = 0.01 * SI().zStep();
    cachedcs.zrg.widen( zrgeps );  
    
    return cachedcs.includes( cs );
}


void Engine::swapCacheAndItsBackup()
{
    const TypeSet<DataPack::ID> tempcachedatapackids = attribcachedatapackids_;
    const ObjectSet<const DataHolder> tempcache = attribcache_;
    const ObjectSet<CacheSpecs> tempcachespecs = attribcachespecs_;
    attribcachedatapackids_ = attribbkpcachedatapackids_;
    attribcache_ = attribbackupcache_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbkpcachedatapackids_ = tempcachedatapackids;
    attribbackupcache_ = tempcache;
    attribbackupcachespecs_ = tempcachespecs;
}


void Engine::updateFlatCubesContainer( const CubeSampling& cs, const int idx,
					bool addremove )
{
    if ( !(cs.nrInl()==1) && !(cs.nrCrl()==1) )
	return;
    
    ObjectSet<FlatCubeInfo>& flatcubes = *flatcubescontainer_[idx];

    int idxinquestion = -1;
    for ( int flatcsidx=0; flatcsidx<flatcubes.size(); flatcsidx++ )
	if ( flatcubes[flatcsidx]->flatcs_.defaultDir() == cs.defaultDir() )
	{
	    if ( flatcubes[flatcsidx]->flatcs_.nrInl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hrg.start.inl == 
			cs.hrg.start.inl )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	    else if ( flatcubes[flatcsidx]->flatcs_.nrCrl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hrg.start.crl ==
		     cs.hrg.start.crl )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	}

    if ( addremove )
    {
	if ( idxinquestion == -1 )
	{
	    FlatCubeInfo* flatcsinfo = new FlatCubeInfo();
	    flatcsinfo->flatcs_.include( cs );
	    flatcubes += flatcsinfo;
	}
	else
	{
	    flatcubes[idxinquestion]->flatcs_.include( cs );
	    flatcubes[idxinquestion]->nrseeds_++;
	}
    }
    else
    {
	if ( idxinquestion == -1 ) return;

	flatcubes[idxinquestion]->nrseeds_--;
	if ( flatcubes[idxinquestion]->nrseeds_ == 0 )
	    flatcubes.remove( idxinquestion );
    }
}


ObjectSet<CubeSampling>* Engine::getTrackedFlatCubes( const int idx ) const
{
    if ( (flatcubescontainer_.size()==0) || !flatcubescontainer_[idx] )
	return 0;

    const ObjectSet<FlatCubeInfo>& flatcubes = *flatcubescontainer_[idx];
    if ( flatcubes.size()==0 )
	return 0;

    ObjectSet<CubeSampling>* flatcbs = new ObjectSet<CubeSampling>;
    for ( int flatcsidx = 0; flatcsidx<flatcubes.size(); flatcsidx++ )
    {
	CubeSampling* cs = new CubeSampling();
	cs->setEmpty();
	cs->include( flatcubes[flatcsidx]->flatcs_ );
	flatcbs->push( cs );
    }
    return flatcbs;
}


ObjectEditor* Engine::getEditor( const EM::ObjectID& id, bool create )
{
    for ( int idx=0; idx<editors_.size(); idx++ )
    {
	if ( editors_[idx]->emObject().id()==id )
	{
	    if ( create )
		editors_[idx]->ref();
	    return editors_[idx];
	}
    }

    if ( !create ) return 0;

    EM::EMObject* emobj = EM::EMM().getObject(id);
    if ( !emobj ) return 0;

    ObjectEditor* editor = EditorFactory().create( emobj->getTypeStr(), *emobj);
    if ( !editor )
	return 0;

    editors_ += editor;
    editor->ref();
    return editor;
}


void Engine::removeEditor( const EM::ObjectID& objid )
{
    ObjectEditor* editor = getEditor( objid, false );
    if ( editor )
    {
	if ( editor->nrRefs() == 1 )
	    editors_ -= editor;
	editor->unRef();
    }
}


const char* Engine::errMsg() const
{ return errmsg_.str(); }


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



BufferString Engine::setupFileName( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj.ptr() ? EM::Surface::getSetupFileName(*ioobj)
		       : BufferString("");
}


void Engine::fillPar( IOPar& iopar ) const
{
    int trackeridx = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	const EMTracker* tracker = trackers_[idx];
	if ( !tracker ) continue;
	
	IOPar localpar;
	localpar.set( sKeyObjectID(),EM::EMM().getMultiID(tracker->objectID()));
	localpar.setYN( sKeyEnabled(), tracker->isEnabled() );

	EMSeedPicker* seedpicker = 
	    		const_cast<EMTracker*>(tracker)->getSeedPicker(false);
	if ( seedpicker ) 
	    localpar.set( sKeySeedConMode(), seedpicker->getSeedConnectMode() );

//	tracker->fillPar( localpar );

	iopar.mergeComp( localpar, toString(trackeridx) );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume_.fillPar( iopar );

    IOPar tppar;
    trackplane_.fillPar( tppar );
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
	if ( tppar ) trackplane_.usePar( *tppar );
    }

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    iopar.get( sKeyNrTrackers(), nrtrackers );

    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	PtrMan<IOPar> localpar = iopar.subselect( toString(idx) );
	if ( !localpar ) continue;
	
	if ( !localpar->get(sKeyObjectID(),midtoload) ) continue;
	EM::ObjectID oid = EM::EMM().getObjectID( midtoload );
	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj )
	{
	    loadEMObject.trigger();
	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	    if ( emobj ) emobj->ref();
	}

	if ( !emobj )
	{
	    PtrMan<Executor> exec =
		EM::EMM().objectLoader( MPE::engine().midtoload );
	    if ( exec ) exec->execute();

	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	    if ( emobj ) emobj->ref();
	}

	if ( !emobj )
	    continue;
	    
	const int trackeridx = addTracker( emobj );
	emobj->unRefNoDelete();
	if ( trackeridx < 0 ) continue;
	EMTracker* tracker = trackers_[trackeridx];
	
	bool doenable = true;
	localpar->getYN( sKeyEnabled(), doenable );
	tracker->enable( doenable );

	int seedconmode = -1;
	localpar->get( sKeySeedConMode(), seedconmode );
	EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( seedpicker && seedconmode!=-1 )
	    seedpicker->setSeedConnectMode( seedconmode );

	// old restore session policy without separate tracking setup file
	tracker->usePar( *localpar );
    }

    return true;
}


void Engine::init()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
    setActiveVolume( getDefaultActiveVolume() );
}

} // namespace MPE
