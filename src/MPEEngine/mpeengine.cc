/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

#include "mpeengine.h"

#include "arrayndimpl.h"
#include "autotracker.h"
#include "emeditor.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurface.h"
#include "emtracker.h"
#include "emundo.h"
#include "envvars.h"
#include "executor.h"
#include "flatposdata.h"
#include "geomelement.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "survinfo.h"

#define mRetErr( msg, retval ) { errmsg_ = msg; return retval; }

MPE::Engine& MPE::engine()
{
    mDefineStaticLocalObject( PtrMan<MPE::Engine>, theinst_,
			      = new MPE::Engine );
    return *theinst_;
}

namespace MPE
{

// MPE::Engine
Engine::Engine()
    : activevolumechange(this)
    , trackeraddremove(this)
    , trackertoberemoved(this)
    , loadEMObject(this)
    , actionCalled(this)
    , actionFinished(this)
    , settingsChanged(this)
    , oneactivetracker_( 0 )
    , activetracker_( 0 )
    , undoeventid_(-1)
    , rdmlinetkpath_(0)
    , rdlid_(-1)
    , state_(Stopped)
    , dpm_(DPM(DataPackMgr::SeisID()))
    , activevolume_(!DBM().isBad())
    , validator_(0)
{
    trackers_.setNullAllowed();
    trackermgrs_.setNullAllowed();
    flatcubescontainer_.setNullAllowed();

    init();
    mAttachCB( DBM().applicationClosing, Engine::applClosingCB );
}


Engine::~Engine()
{
    detachAllNotifiers();

    deepErase( trackermgrs_ );
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );

    for ( int idx=attribcachedatapackids_.size()-1; idx>=0; idx-- )
	dpm_.unRef( attribcachedatapackids_[idx] );
    for ( int idx=attribbkpcachedatapackids_.size()-1; idx>=0; idx-- )
	dpm_.unRef( attribbkpcachedatapackids_[idx] );

    setValidator( 0 );
}


void Engine::setValidator( TrackSettingsValidator* val )
{ delete validator_; validator_ = val; }


const TrcKeyZSampling& Engine::activeVolume() const
{ return activevolume_; }


void Engine::setActiveVolume( const TrcKeyZSampling& nav )
{
    activevolume_ = nav;
    activevolumechange.trigger();
}


void Engine::setActive2DLine( Pos::GeomID geomid )
{ activegeomid_ = geomid; }

Pos::GeomID Engine::activeGeomID() const
{ return activegeomid_; }


void Engine::updateSeedOnlyPropagation( bool yn )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	SectionTracker* sectiontracker =
		    trackers_[idx]->getSectionTracker( true );
	sectiontracker->setSeedOnlyPropagation( yn );
    }
}


bool Engine::startTracking( uiString& errmsg )
{
    errmsg.setEmpty();
    if ( state_ == Started )
	return false;

    if ( !prepareForTrackInVolume(errmsg) )
	return false;

    state_ = Started;
    actionCalled.trigger();
    return trackInVolume();
}


bool Engine::startRetrack( uiString& errmsg )
{
    errmsg.setEmpty();
    if ( state_ == Started )
	return false;

    if ( !prepareForRetrack() )
	return false;

    return startTracking( errmsg );
}


bool Engine::trackingInProgress() const
{
    for ( int idx=0; idx<trackermgrs_.size(); idx++ )
    {
	const HorizonTrackerMgr* htm = trackermgrs_[idx];
	if ( htm && htm->hasTasks() ) return true;
    }

    return false;
}


void Engine::stopTracking()
{
    for ( int idx=0; idx<trackermgrs_.size(); idx++ )
    {
	HorizonTrackerMgr* htm = trackermgrs_[idx];
	if ( !htm ) continue;

	htm->stop();
    }

    state_ = Stopped;
    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::trackingFinishedCB( CallBacker* )
{
    const EM::Object* emobj = getCurrentEMObject();
    if ( !emobj ) return;

    Undo& emundo = EM::MGR().undo(emobj->id());
    const int currentevent = emundo.currentEventID();
    if ( currentevent != undoeventid_ )
	emundo.setUserInteractionEnd( currentevent );

    state_ = Stopped;
    actionCalled.trigger();
    actionFinished.trigger();
}


bool Engine::canUnDo()
{
    const EM::Object* emobj = getCurrentEMObject();
    if ( !emobj ) return false;

    return EM::MGR().undo(emobj->id()).canUnDo();
}


bool Engine::canReDo()
{
    const EM::Object* emobj = getCurrentEMObject();
    if ( !emobj ) return false;

    return EM::MGR().undo(emobj->id()).canReDo();
}


EM::Object* Engine::getCurrentEMObject() const
{
   if ( !activetracker_ )
	return 0;

    return EM::MGR().getObject( activetracker_->objectID() );

}


void Engine::undo( uiString& errmsg )
{
    EM::Object* emobj = getCurrentEMObject();
    if ( !emobj ) return;

    mDynamicCastGet( EM::Undo*,emundo,&EM::MGR().undo(emobj->id()) )
    if ( !emundo ) return;

    emobj->ref();
    emobj->setBurstAlert( true );

    if ( !emundo->unDo(1,true) )
	errmsg = tr("Cannot undo everything.");

    emobj->setBurstAlert( false );
    emobj->unRef();

    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::redo( uiString& errmsg )
{
    EM::Object* emobj = getCurrentEMObject();
    if(!emobj) return;

    mDynamicCastGet( EM::Undo*,emundo,&EM::MGR().undo(emobj->id()) )
    if( !emundo ) return;

    emobj->ref();
    emobj->setBurstAlert( true );

    if ( !emundo->reDo(1,true) )
	errmsg = tr("Cannot redo everything.");

    emobj->setBurstAlert( false );
    emobj->unRef();

    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::enableTracking( bool yn )
{
    if ( !activetracker_ ) return;

    activetracker_->enable( yn );
    actionCalled.trigger();
}


bool Engine::prepareForTrackInVolume( uiString& errmsg )
{
    if ( validator_ && !validator_->checkActiveTracker() )
	return false;

    EMSeedPicker* seedpicker = activetracker_->getSeedPicker( true );
    if ( !seedpicker ||
	 seedpicker->getTrackMode()!=EMSeedPicker::TrackFromSeeds )
	return false;

    DBKey key;
    Attrib::SelSpec as( *seedpicker->getSelSpec() );
    if ( validator_ && !validator_->checkStoredData(as,key) )
	return false;

    if ( validator_ && !validator_->checkPreloadedData(key) )
	return false;

    auto rsdp = Seis::PLDM().get<RegularSeisDataPack>( key );
    setAttribData( as, rsdp->id() );
    setActiveVolume( TrcKeyZSampling(rsdp->subSel()) );
    return true;
}


bool Engine::prepareForRetrack()
{
    if ( !activetracker_ || !activetracker_->emObject() )
	return false;

    EMSeedPicker* seedpicker = activetracker_->getSeedPicker( true );
    if ( !seedpicker ) return false;

    EM::Object* emobj = activetracker_->emObject();
    emobj->setBurstAlert( true );
    emobj->removeAllUnSeedPos();

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( hor3d ) hor3d->initAllAuxData();

    seedpicker->reTrack();
    emobj->setBurstAlert( false );
    return true;
}


bool Engine::trackInVolume()
{
    EMTracker* tracker = activetracker_;
    if ( !tracker || !tracker->isEnabled() )
	return false;

    DBKey oid = tracker->objectID();
    EM::Object* emobj = EM::MGR().getObject( oid );
    if ( !emobj || emobj->isLocked() )
	return false;

    emobj->geometryElement()->blockCallBacks(true);
    EMSeedPicker* seedpicker = tracker->getSeedPicker( false );
    if ( !seedpicker ) return false;

    TypeSet<TrcKey> seeds;
    seedpicker->getSeeds( seeds );
    const int trackeridx = trackers_.indexOf( tracker );
    if ( !trackermgrs_.validIdx(trackeridx) )
	return false;

    HorizonTrackerMgr* htm = trackermgrs_[trackeridx];
    if ( !htm )
    {
	htm = new HorizonTrackerMgr( *tracker );
	htm->finished.notify( mCB(this,Engine,trackingFinishedCB) );
	delete trackermgrs_.replace( trackeridx, htm );
    }

    actionCalled.trigger();

//    EM::MGR().undo().removeAllBeforeCurrentEvent();
    undoeventid_ = EM::MGR().undo(emobj->id()).currentEventID();
    htm->setSeeds( seeds );
    htm->startFromSeeds();
    return true;
}


void Engine::removeSelectionInPolygon( const Selector<Coord3>& selector,
				       const TaskRunnerProvider& trprov )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	const DBKey oid = trackers_[idx]->objectID();
	EM::MGR().removeSelected( oid, selector, trprov );

	EM::Object* emobj = EM::MGR().getObject( oid );
	if ( !emobj->getRemovedPolySelectedPosBox().isEmpty() )
	{
	    emobj->emptyRemovedPolySelectedPosBox();
	}
    }
}


void Engine::getAvailableTrackerTypes( BufferStringSet& res ) const
{ res = EMTracker::factory().getKeys(); }


static void showRefCountInfo( EMTracker* tracker )
{
    mDefineStaticLocalObject(bool,yn,= GetEnvVarYN("OD_DEBUG_TRACKERS"));
    if ( yn )
    {
	EM::Object* emobj = tracker ? tracker->emObject() : 0;
	BufferString msg = emobj ? emobj->name() : "<unknown>";
	msg.add( " - refcount=" ).add( tracker ? tracker->nrRefs() : -1 );
	DBG::message( msg );
    }
}


int Engine::addTracker( EM::Object* emobj )
{
    if ( !emobj )
	mRetErr( "No valid object", -1 );

    const int idx = getTrackerByObject( emobj->id() );

    if ( idx != -1 )
    {
	trackers_[idx]->ref();
	showRefCountInfo( trackers_[idx] );
	activetracker_ = trackers_[idx];
	return idx;
    }

    EMTracker* tracker = EMTracker::factory().create( emobj->getTypeStr(),
						      emobj );
    if ( !tracker )
	mRetErr( "Cannot find this trackertype", -1 );

    tracker->ref();
    showRefCountInfo( tracker );
    activetracker_ = tracker;
    trackers_ += tracker;
    trackermgrs_ += 0;
    ObjectSet<FlatCubeInfo>* flatcubes = new ObjectSet<FlatCubeInfo>;
    flatcubescontainer_ += flatcubes;
    trackeraddremove.trigger();

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( hor3d )
    {
	hor3d->initTrackingArrays();
	hor3d->initTrackingAuxData();
    }

    return trackers_.size()-1;
}


void Engine::removeTracker( int idx )
{
    if ( trackingInProgress() )
	return;

    if ( !trackers_.validIdx(idx) )
	return;

    EMTracker* tracker = trackers_[idx];
    if ( !tracker ) return;

    const int noofref = tracker->nrRefs();
    if ( noofref==1 )
	trackertoberemoved.trigger( idx );

    tracker->unRef();
    showRefCountInfo( noofref>1 ? tracker : 0 );
    if ( noofref != 1 )
	return;

    if ( activetracker_ == tracker )
	activetracker_ = 0;

    trackers_.replace( idx, 0 );
    delete trackermgrs_.replace( idx, 0 );

    deepErase( *flatcubescontainer_[idx] );
    flatcubescontainer_.replace( idx, 0 );

    if ( nrTrackersAlive()==0 )
	activevolume_.setEmpty();

    trackeraddremove.trigger();
}


void Engine::refTracker( const DBKey& emid )
{
    const int idx = getTrackerByObject( emid );
    if ( trackers_.validIdx(idx) && trackers_[idx] )
    {
	trackers_[idx]->ref();
	showRefCountInfo( trackers_[idx] );
    }
}


void Engine::unRefTracker( const DBKey& emid, bool nodelete )
{
    const int idx = getTrackerByObject( emid );
    if ( !trackers_.validIdx(idx) || !trackers_[idx] )
	return;

    if ( nodelete )
    {
	trackers_[idx]->unRefNoDelete();
	showRefCountInfo( trackers_[idx] );
	return;
    }

    removeTracker( idx );
}


bool Engine::hasTracker( const DBKey& emid ) const
{
    const int idx = getTrackerByObject( emid );
    return trackers_.validIdx(idx) && trackers_[idx];
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


int Engine::getTrackerByObject( const DBKey& oid ) const
{
    if ( oid.isInvalid() ) return -1;

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

	if ( trackers_[idx]->objectName() == objname )
	    return idx;
    }

    return -1;
}


void Engine::setActiveTracker( EMTracker* tracker )
{ activetracker_ = tracker; }


void Engine::setActiveTracker( const DBKey& objid )
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


void Engine::getNeededAttribs( Attrib::SelSpecList& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers_.size(); trackeridx++ )
    {
	const EMTracker* tracker = trackers_[trackeridx];
	if ( !tracker ) continue;

	if ( oneactivetracker_ && oneactivetracker_!=tracker )
	    continue;

	Attrib::SelSpecList specs;
	tracker->getNeededAttribs( specs );
	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec& as = specs[idx];
	    res.addIfNew( as );
	}
    }
}


TrcKeyZSampling Engine::getAttribCube( const Attrib::SelSpec& as ) const
{
    TrcKeyZSampling res( engine().activeVolume() );
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;
	const TrcKeyZSampling cs = trackers_[idx]->getAttribCube( as );
	res.include(cs);
    }

    return res;
}


bool Engine::pickingOnSameData( const Attrib::SelSpec& oldss,
				const Attrib::SelSpec& newss,
				uiString& error ) const
{
    const FixedString defstr = oldss.defString();
    const bool match = defstr == newss.defString();
    if ( match ) return true;

    // TODO: Other messages for other options
    error = tr("This horizon has previously been picked on:\n'%1'.\n"
		"The new seed has been picked on:\n'%2'.")
			 .arg(oldss.userRef())
			 .arg(newss.userRef());
    return false;
}


bool Engine::isSelSpecSame( const Attrib::SelSpec& setupss,
			    const Attrib::SelSpec& clickedss ) const
{
    return setupss.id()==clickedss.id() &&
	   setupss.isNLA()==clickedss.isNLA() &&
	   BufferString(setupss.defString())==
	       BufferString(clickedss.defString()) &&
	    setupss.is2D()==clickedss.is2D();
}


int Engine::getCacheIndexOf( const Attrib::SelSpec& as ) const
{
    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	if ( attribcachespecs_[idx]->attrsel_.is2D()	!= as.is2D()  ||
	     attribcachespecs_[idx]->attrsel_.isNLA()	!= as.isNLA() ||
	     attribcachespecs_[idx]->attrsel_.id()	!= as.id() )
	    continue;

	if ( !as.is2D() )
	    return idx;

	if ( attribcachespecs_[idx]->geomid_ != activeGeomID() )
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


bool Engine::hasAttribCache( const Attrib::SelSpec& as ) const
{
    const DataPack::ID dpid = getAttribCacheID( as );
    return dpm_.isPresent( dpid );
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    DataPack::ID cacheid )
{
    auto regfdp = DPM(DataPackMgr::FlatID()).get<SeisFlatDataPack>( cacheid );
    if ( regfdp )
	cacheid = regfdp->getSourceDataPack().id();

    const int idx = getCacheIndexOf(as);
    if ( attribcachedatapackids_.validIdx(idx) )
    {
	if ( cacheid == DataPack::cNoID() )
	{
	    dpm_.unRef( attribcachedatapackids_[idx] );
	    attribcachedatapackids_.removeSingle( idx );
	    delete attribcachespecs_.removeSingle( idx );
	}
	else
	{
	    auto newdata= dpm_.get<VolumeDataPack>(cacheid);
	    if ( newdata )
	    {
		dpm_.unRef( attribcachedatapackids_[idx] );
		attribcachedatapackids_[idx] = cacheid;
		newdata->ref();
	    }
	}
    }
    else if ( cacheid != DataPack::cNoID() )
    {
	auto newdata = dpm_.get<VolumeDataPack>( cacheid );
	if ( newdata )
	{
	    attribcachespecs_ += as.is2D() ?
		new CacheSpecs( as, activeGeomID() ) :
		new CacheSpecs( as ) ;

	    attribcachedatapackids_ += cacheid;
	    newdata->ref();
	}
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as,
			    const TrcKeyZSampling& cs )
{
    auto rsdp = dpm_.get<RegularSeisDataPack>( getAttribCacheID(as) );
    if ( !rsdp )
	return false;

    TrcKeyZSampling cachedcs( rsdp->subSel() );
    const float zrgeps = 0.01f * SI().zStep();
    cachedcs.zsamp_.widen( zrgeps );
    return cachedcs.includes( cs );
}


void Engine::swapCacheAndItsBackup()
{
    const TypeSet<DataPack::ID> tempcachedatapackids = attribcachedatapackids_;
    const ObjectSet<CacheSpecs> tempcachespecs = attribcachespecs_;
    attribcachedatapackids_ = attribbkpcachedatapackids_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbkpcachedatapackids_ = tempcachedatapackids;
    attribbackupcachespecs_ = tempcachespecs;
}


void Engine::updateFlatCubesContainer( const TrcKeyZSampling& cs, int idx,
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
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.inl() ==
			cs.hsamp_.start_.inl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	    else if ( flatcubes[flatcsidx]->flatcs_.nrCrl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.crl() ==
		     cs.hsamp_.start_.crl() )
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
	    flatcubes.removeSingle( idxinquestion );
    }
}


ObjectSet<TrcKeyZSampling>* Engine::getTrackedFlatCubes( const int idx ) const
{
    if ( (flatcubescontainer_.size()==0) || !flatcubescontainer_[idx] )
	return 0;

    const ObjectSet<FlatCubeInfo>& flatcubes = *flatcubescontainer_[idx];
    if ( flatcubes.size()==0 )
	return 0;

    ObjectSet<TrcKeyZSampling>* flatcbs = new ObjectSet<TrcKeyZSampling>;
    for ( int flatcsidx = 0; flatcsidx<flatcubes.size(); flatcsidx++ )
    {
	TrcKeyZSampling* cs = new TrcKeyZSampling();
	cs->setEmpty();
	cs->include( flatcubes[flatcsidx]->flatcs_ );
	flatcbs->push( cs );
    }
    return flatcbs;
}


DataPack::ID Engine::getSeedPosDataPack( const TrcKey& tk, float z, int nrtrcs,
					const StepInterval<float>& zintv ) const
{
    Attrib::SelSpecList specs; getNeededAttribs( specs );
    if ( specs.isEmpty() ) return DataPack::cNoID();

    DataPackMgr& dpm = DPM( DataPackMgr::SeisID() );
    const DataPack::ID pldpid = getAttribCacheID( specs[0] );
    auto vdp = dpm.get<VolumeDataPack>( pldpid );
    if ( !vdp )
	return DataPack::cNoID();

    const int globidx = vdp->getNearestGlobalIdx( tk );
    if ( globidx < 0 ) return DataPack::cNoID();

    StepInterval<float> zintv2 = zintv; zintv2.step = vdp->zRange().step;
    const int nrz = zintv2.nrSteps() + 1;
    Array2DImpl<float>* seeddata = new Array2DImpl<float>( nrtrcs, nrz );
    seeddata->setAll( mUdf(float) );

    const int trcidx0 = globidx - (int)(nrtrcs/2);
    const auto zsamp = vdp->zRange();
    const int zidx0 = zsamp.getIndex( z + zintv.start );
    for ( int tidx=0; tidx<nrtrcs; tidx++ )
    {
	const int curtrcidx = trcidx0+tidx;
	if ( curtrcidx<0 || curtrcidx >= vdp->nrPositions() )
	    continue;

	const OffsetValueSeries<float> ovs =
			    vdp->getTrcStorage( 0, trcidx0+tidx );
	for ( int zidx=0; zidx<nrz; zidx++ )
	{
	    const float val = ovs[zidx0+zidx];
	    seeddata->set( tidx, zidx, val );
	}
    }

    StepInterval<double> trcrg;
    trcrg.start = tk.trcNr() - (nrtrcs)/2;
    trcrg.stop = tk.trcNr() + (nrtrcs)/2;
    StepInterval<double> zrg;
    zrg.start = mCast(double,zsamp.atIndex(zidx0));
    zrg.stop = mCast(double,zsamp.atIndex(zidx0+nrz-1));
    zrg.step = mCast(double,zsamp.step);

    FlatDataPack* fdp = new FlatDataPack( "Seismics", seeddata );
    fdp->posData().setRange( true, trcrg );
    fdp->posData().setRange( false, zrg );
    DPM(DataPackMgr::FlatID()).add( fdp );
    return fdp->id();
}


ObjectEditor* Engine::getEditor( const DBKey& id, bool create )
{
    for ( int idx=0; idx<editors_.size(); idx++ )
    {
	if ( editors_[idx]->emObject().id()==id )
	{
	    if ( create ) editors_[idx]->addUser();
	    return editors_[idx];
	}
    }

    if ( !create ) return 0;

    EM::Object* emobj = EM::getMgr(id).getObject(id);
    if ( !emobj ) return 0;

    ObjectEditor* editor = ObjectEditor::factory().create(
					emobj->getTypeStr(), *emobj );
    if ( !editor )
	return 0;

    editors_ += editor;
    editor->ref();
    editor->addUser();
    return editor;
}


void Engine::removeEditor( const DBKey& objid )
{
    ObjectEditor* editor = getEditor( objid, false );
    if ( editor )
    {
	editor->removeUser();
	if ( editor->nrUsers() == 0 )
	{
	    editors_ -= editor;
	    editor->unRef();
	}
    }
}


const char* Engine::errMsg() const
{ return errmsg_.str(); }


BufferString Engine::setupFileName( const DBKey& emid ) const
{
    PtrMan<IOObj> ioobj = emid.getIOObj();
    return ioobj ? EM::Surface::getSetupFileName(*ioobj) : BufferString("");
}


void Engine::fillPar( IOPar& iopar ) const
{
    int trackeridx = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	const EMTracker* tracker = trackers_[idx];
	if ( !tracker ) continue;

	IOPar localpar;
	localpar.set( sKeyObjectID(), tracker->objectID() );
	localpar.setYN( sKeyEnabled(), tracker->isEnabled() );

	EMSeedPicker* seedpicker =
			const_cast<EMTracker*>(tracker)->getSeedPicker(false);
	if ( seedpicker )
	    localpar.set( sKeySeedConMode(), seedpicker->getTrackMode() );

//	tracker->fillPar( localpar );

	iopar.mergeComp( localpar, toString(trackeridx) );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume_.fillPar( iopar );
}


bool Engine::usePar( const IOPar& iopar )
{
    init();

    TrcKeyZSampling newvolume;
    if ( newvolume.usePar(iopar) )
	setActiveVolume( newvolume );

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    iopar.get( sKeyNrTrackers(), nrtrackers );

    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	PtrMan<IOPar> localpar = iopar.subselect( toString(idx) );
	if ( !localpar ) continue;

	if ( !localpar->get(sKeyObjectID(),emidtoload_) ) continue;
	EM::Object* emobj = EM::MGR().getObject( emidtoload_ );
	if ( !emobj )
	{
	    loadEMObject.trigger();
	    emobj = EM::MGR().getObject( emidtoload_ );
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
	    seedpicker->setTrackMode( (EMSeedPicker::TrackMode)seedconmode );

	// old restore session policy without separate tracking setup file
	tracker->usePar( *localpar );
    }

    return true;
}


void Engine::init()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
    deepErase( trackermgrs_ );

    for ( int idx=0; idx<attribcachedatapackids_.size(); idx++ )
	dpm_.unRef( attribcachedatapackids_[idx] );
    for ( int idx=0; idx<attribbkpcachedatapackids_.size(); idx++ )
	dpm_.unRef( attribbkpcachedatapackids_[idx] );

    attribcachedatapackids_.erase();
    attribbkpcachedatapackids_.erase();
    activevolume_.init( false );
}


void Engine::applClosingCB( CallBacker* )
{
    for ( int idx=trackers_.size()-1; idx>=0; idx-- )
	removeTracker( idx );
}


} // namespace MPE
