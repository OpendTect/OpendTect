/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeengine.h"

#include "arrayndimpl.h"
#include "autotracker.h"
#include "emeditor.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurface.h"
#include "emundo.h"
#include "envvars.h"
#include "executor.h"
#include "flatposdata.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "sectiontracker.h"
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

// TrackSettingsValidator
TrackSettingsValidator::TrackSettingsValidator()
{}


TrackSettingsValidator::~TrackSettingsValidator()
{}


// MPE::Engine
Engine::Engine()
    : activevolumechange(this)
    , actionCalled(this)
    , actionFinished(this)
    , trackeradded(this)
    , settingsChanged(this)
{
    activevolume_.init( false );
    mAttachCB( IOM().surveyChanged, Engine::surveyChangedCB );
}


Engine::~Engine()
{
    detachAllNotifiers();

    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( trackerpars_ );
}


void Engine::cleanup()
{
    trackers_.erase();
    editors_.erase();
    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    activevolume_.init( false );
    deepErase( trackerpars_ );
    trackermids_.erase();
}


void Engine::setValidator( TrackSettingsValidator* val )
{
    delete validator_;
    validator_ = val;
}


const TrcKeyPath* Engine::activePath() const
{
    return rdmlinetkpath_;
}


void Engine::setActivePath( const TrcKeyPath* tkp )
{
    rdmlinetkpath_ = tkp;
}


RandomLineID Engine::activeRandomLineID() const
{
    return rdlid_;
}


void Engine::setActiveRandomLineID( const RandomLineID& rdlid )
{
    rdlid_ = rdlid;
}


const TrcKeyZSampling& Engine::activeVolume() const
{
    return activevolume_;
}


void Engine::setActiveVolume( const TrcKeyZSampling& nav )
{
    activevolume_ = nav;
    activegeomid_ = nav.hsamp_.getGeomID();
    activevolumechange.trigger();
}


void Engine::setActive2DLine( const Pos::GeomID& geomid )
{
    activegeomid_ = geomid;
}


Pos::GeomID Engine::activeGeomID() const
{
    return activegeomid_;
}


void Engine::updateSeedOnlyPropagation( bool yn )
{
    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	RefMan<EMTracker> tracker = trackers_[idx];
	if ( !tracker || !tracker->isEnabled() )
	    continue;

	SectionTracker* sectiontracker =
			    trackers_[idx]->getSectionTracker( true );
	if ( sectiontracker )
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


bool Engine::startFromEdges( uiString& errmsg )
{
    errmsg.setEmpty();
    if ( state_ == Started )
	return false;

    if ( !prepareForTrackInVolume(errmsg) )
	return false;

    state_ = Started;
    actionCalled.trigger();
    return trackFromEdges();
}


bool Engine::trackingInProgress() const
{
    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	ConstRefMan<EMTracker> tracker = trackers_[idx];
	if ( tracker && tracker->trackingInProgress() )
	    return true;
    }

    return false;
}


void Engine::stopTracking()
{
    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	RefMan<EMTracker> tracker = trackers_[idx];
	if ( tracker )
	    tracker->stopTracking();
    }

    state_ = Stopped;
    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::surveyChangedCB( CallBacker* )
{
    cleanup();
}


void Engine::trackingFinishedCB( CallBacker* )
{
    ConstRefMan<EM::EMObject> emobj = getCurrentEMObject();
    if ( !emobj )
	return;

    Undo& emundo = EM::EMM().undo( emobj->id() );

    const int currentevent = emundo.currentEventID();
    if ( currentevent != undoeventid_ )
	emundo.setUserInteractionEnd( currentevent );

    state_ = Stopped;
    actionCalled.trigger();
    actionFinished.trigger();
}


ConstRefMan<EMTracker> Engine::getOneActiveTracker() const
{
    return oneactivetracker_.get();
}


RefMan<EM::EMObject> Engine::getCurrentEMObject() const
{
    ConstRefMan<EMTracker> activetracker = getActiveTracker();
    if ( !activetracker )
	return nullptr;

    return EM::EMM().getObject( activetracker->objectID() );
}


bool Engine::canUnDo()
{
    ConstRefMan<EM::EMObject> emobj = getCurrentEMObject();
    return emobj ? EM::EMM().undo( emobj->id() ).canUnDo() : false;
}


bool Engine::canReDo()
{
    ConstRefMan<EM::EMObject> emobj = getCurrentEMObject();
    return emobj ? EM::EMM().undo( emobj->id() ).canReDo() : false;
}


void Engine::undo( uiString& errmsg )
{
    RefMan<EM::EMObject> emobj = getCurrentEMObject();
    if ( !emobj )
	return;

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo( emobj->id() ) )
    if ( !emundo )
	return;

    emobj->setBurstAlert( true );
    if ( !emundo->unDo(1,true) )
	errmsg = tr("Could not undo everything.");

    emobj->setBurstAlert( false );
    emobj = nullptr;
    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::redo( uiString& errmsg )
{
    RefMan<EM::EMObject> emobj = getCurrentEMObject();
    if ( !emobj )
	return;

    mDynamicCastGet( EM::EMUndo*, emundo, &EM::EMM().undo( emobj->id() ) )
    if ( !emundo )
	return;

    emobj->setBurstAlert( true );
    if ( !emundo->reDo(1,true) )
	errmsg = tr("Could not redo everything.");

    emobj->setBurstAlert( false );
    emobj = nullptr;
    actionCalled.trigger();
    actionFinished.trigger();
}


void Engine::enableTracking( bool yn )
{
    RefMan<EMTracker> activetracker = getActiveTracker();
    if ( !activetracker )
	return;

    activetracker->enable( yn );
    actionCalled.trigger();
}


bool Engine::prepareForTrackInVolume( uiString& )
{
    RefMan<EMTracker> activetracker = getActiveTracker();
    if ( (validator_ && !validator_->checkActiveTracker()) || !activetracker )
	return false;

    EMSeedPicker* seedpicker = activetracker->getSeedPicker( true );
    if ( !seedpicker ||
	 seedpicker->getTrackMode()!=EMSeedPicker::TrackFromSeeds )
	return false;

    MultiID key = MultiID::udf();
    Attrib::SelSpec as( *seedpicker->getSelSpec() );
    if ( validator_ && !validator_->checkStoredData(as,key) )
	return false;

    if ( validator_ && !validator_->checkPreloadedData(key) )
	return false;

    ConstRefMan<RegularSeisDataPack> seisdp =
				Seis::PLDM().get<RegularSeisDataPack>( key );
    if ( !seisdp )
	return false;

    setAttribData( as, *seisdp.ptr() );
    setActiveVolume( seisdp->sampling() );
    return true;
}


bool Engine::prepareForRetrack()
{
    RefMan<EMTracker> activetracker = getActiveTracker();
    if ( !activetracker )
	return false;

    RefMan<EM::EMObject> emobject = activetracker->emObject();
    EMSeedPicker* seedpicker = activetracker->getSeedPicker( true );
    if ( !emobject || !seedpicker )
	return false;

    emobject = activetracker->emObject();
    emobject->setBurstAlert( true );
    emobject->removeAllUnSeedPos();

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr())
    if ( hor3d )
	hor3d->initAllAuxData();

    seedpicker->reTrack();
    emobject->setBurstAlert( false );
    return true;
}


bool Engine::trackInVolume()
{
    RefMan<EMTracker> activetracker = getActiveTracker();
    if ( !activetracker || activetracker->is2D() || !activetracker->isEnabled())
	return false;

    const EM::ObjectID oid = activetracker->objectID();
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( oid );
    if ( !emobj || emobj->isLocked() )
	return false;

    emobj->geometryElement()->blockCallBacks( true );
    EMSeedPicker* seedpicker = activetracker->getSeedPicker( false );
    if ( !seedpicker )
	return false;

    TypeSet<TrcKey> seeds;
    seedpicker->getSeeds( seeds );
    const int trackeridx = trackers_.indexOf( activetracker.ptr() );
    if ( !trackers_.validIdx(trackeridx) )
	return false;

    if ( !activetracker->hasTrackingMgr() )
	activetracker->createMgr();

    mAttachCBIfNotAttached( activetracker->trackingFinished,
			    Engine::trackingFinishedCB );
    actionCalled.trigger();

//    EM::EMM().undo().removeAllBeforeCurrentEvent();
    undoeventid_ = EM::EMM().undo( emobj->id() ).currentEventID();
    activetracker->startFromSeeds( seeds );
    return true;
}


bool Engine::trackFromEdges()
{
    return true;
}


void Engine::removeSelectionInPolygon( const Selector<Coord3>& selector,
				       TaskRunner* taskr )
{
    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	RefMan<EMTracker> tracker = trackers_[idx];
	if ( !tracker || !tracker->isEnabled() )
	    continue;

	const EM::ObjectID oid = tracker->objectID();
	EM::EMM().removeSelected( oid, selector, taskr );

	RefMan<EM::EMObject> emobj = EM::EMM().getObject( oid );
	if ( emobj && !emobj->getRemovedPolySelectedPosBox().isEmpty() )
	    emobj->emptyRemovedPolySelectedPosBox();
    }
}


bool Engine::hasTracker() const
{
    trackers_.cleanupNullPtrs();
    return trackers_.size() > 0;
}


bool Engine::hasTracker( const EM::ObjectID& emid ) const
{
    ConstRefMan<EMTracker> tracker = getTrackerByID( emid );
    return tracker;
}


bool Engine::isActiveTracker( const EM::ObjectID& emid ) const
{
    ConstRefMan<EMTracker> activetracker = getActiveTracker();
    return activetracker && activetracker->objectID() == emid;
}


void Engine::setActiveTracker( EMTracker* tracker )
{
    if ( !tracker )
    {
	activetracker_ = nullptr;
	return;
    }
    else if ( hasTracker(tracker->objectID()) )
    {
	activetracker_ = tracker;
	return;
    }

    activetracker_ = tracker;
    trackers_ += tracker;
    tracker->initTrackingMgr();
    trackeradded.trigger( tracker->objectID() );
}


ConstRefMan<EMTracker> Engine::getActiveTracker() const
{
    return activetracker_.get();
}


RefMan<EMTracker> Engine::getActiveTracker()
{
    return activetracker_.get();
}


ConstRefMan<EMTracker> Engine::getTrackerByID( const EM::ObjectID& emid ) const
{
    return mSelf().getTrackerByID( emid );
}


RefMan<EMTracker> Engine::getTrackerByID( const EM::ObjectID& emid )
{
    if ( !emid.isValid() )
	return nullptr;

    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	RefMan<EMTracker> tracker = trackers_[idx];
	if ( tracker && tracker->objectID() == emid )
	    return tracker;
    }

    return nullptr;
}


void Engine::getTrackerIDsByType( const char* typestr,
				  TypeSet<EM::ObjectID>& emids ) const
{
    trackers_.cleanupNullPtrs();
    const StringView reqtype( typestr );
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	ConstRefMan<EMTracker> tracker = trackers_[idx];
	ConstRefMan<EM::EMObject> emobject = tracker ? tracker->emObject()
						     : nullptr;
	if ( emobject && reqtype == emobject->getTypeStr() )
	    emids += emobject->id();
    }
}


void Engine::setOneActiveTracker( const EMTracker* tracker )
{
    oneactivetracker_ = const_cast<EMTracker*>( tracker );
}


void Engine::unsetOneActiveTracker()
{
    oneactivetracker_ = nullptr;
}


bool Engine::hasEditor( const EM::ObjectID& emid ) const
{
    ConstRefMan<ObjectEditor> editor = getEditorByID( emid );
    return editor;
}


ConstRefMan<ObjectEditor> Engine::getEditorByID( const EM::ObjectID& id ) const
{
    return mSelf().getEditorByID( id );
}


RefMan<ObjectEditor> Engine::getEditorByID( const EM::ObjectID& emid )
{
    if ( !emid.isValid() )
	return nullptr;

    editors_.cleanupNullPtrs();
    for ( int idx=0; idx<editors_.size(); idx++ )
    {
	RefMan<ObjectEditor> editor = editors_[idx];
	if ( editor && editor->objectID() == emid )
	    return editor;
    }

    return nullptr;
}


void Engine::getNeededAttribs( TypeSet<Attrib::SelSpec>& res ) const
{
    trackers_.cleanupNullPtrs();
    ConstRefMan<EMTracker> oneactivetracker = getOneActiveTracker();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	ConstRefMan<EMTracker> tracker = trackers_[idx];
	if ( !tracker )
	    continue;

	if ( oneactivetracker && tracker.ptr() != oneactivetracker.ptr() )
	    continue;

	TypeSet<Attrib::SelSpec> specs;
	tracker->getNeededAttribs( specs );
	for ( const auto& as : specs )
	    res.addIfNew( as );
    }
}


TrcKeyZSampling Engine::getAttribCube( const Attrib::SelSpec& as ) const
{
    TrcKeyZSampling res( engine().activeVolume() );
    trackers_.cleanupNullPtrs();
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	ConstRefMan<EMTracker> tracker = trackers_[idx];
	if ( !tracker )
	    continue;

	const TrcKeyZSampling cs = tracker->getAttribCube( as );
	res.include( cs );
    }

    return res;
}


bool Engine::pickingOnSameData( const Attrib::SelSpec& oldss,
				const Attrib::SelSpec& newss,
				uiString& error ) const
{
    const StringView defstr = oldss.defString();
    const bool match = defstr == newss.defString();
    if ( match )
	return true;

    // TODO: Other messages for other options
    error = tr( "This horizon has previously been picked on:\n'%1'.\n"
		"The new seed has been picked on:\n'%2'." )
			 .arg(oldss.userRef())
			 .arg(newss.userRef());
    return false;
}


bool Engine::isSelSpecSame( const Attrib::SelSpec& setupss,
			    const Attrib::SelSpec& clickedss ) const
{
    return setupss.id().asInt()==clickedss.id().asInt() &&
	setupss.isStored()==clickedss.id().isStored() &&
	setupss.isNLA()==clickedss.isNLA() &&
	BufferString(setupss.defString())==
	   BufferString(clickedss.defString()) &&
	setupss.is2D()==clickedss.is2D();
}


int Engine::getCacheIndexOf( const Attrib::SelSpec& as ) const
{
    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	const CacheSpecs& cachespecs = *attribcachespecs_[idx];
	if ( cachespecs.attrsel_.is2D()		!= as.is2D()  ||
	     cachespecs.attrsel_.isNLA()	!= as.isNLA() ||
	     cachespecs.attrsel_.id().asInt()	!= as.id().asInt() ||
	     cachespecs.attrsel_.id().isStored() != as.isStored() )
	    continue;

	if ( cachespecs.geomid_ != activeGeomID() )
	    continue;

	if ( as.is2D() )
	    return idx;

	ConstRefMan<SeisDataPack> seisdp = cachespecs.seisdp_.get();
	if ( !seisdp )
	    continue;

	mDynamicCastGet(const RegularSeisDataPack*,regseisdp,seisdp.ptr())
	if ( regseisdp )
	{
	    const TrcKeySampling cachedtkzs = regseisdp->sampling().hsamp_;
	    if ( cachedtkzs.includes(activevolume_.hsamp_) )
		return idx;
	    else
		continue;
	}

	mDynamicCastGet(const RandomSeisDataPack*,randomdp,seisdp.ptr())
	if ( randomdp )
	    return idx;
    }

    return -1;
}


ConstRefMan<SeisDataPack> Engine::getAttribCacheDP(
					const Attrib::SelSpec& as ) const
{
    const int idx = getCacheIndexOf(as);
    if ( !attribcachespecs_.validIdx(idx) )
	return nullptr;

    return attribcachespecs_.get( idx )->seisdp_.get();
}


bool Engine::hasAttribCache( const Attrib::SelSpec& as ) const
{
    ConstRefMan<SeisDataPack> seisdp = getAttribCacheDP( as );
    return seisdp.ptr();
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    const FlatDataPack& cachedp )
{
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,&cachedp);
    if ( !seisfdp )
	return false;

    ConstRefMan<SeisDataPack> sourcedp = seisfdp->getSource();
    return sourcedp ? setAttribData_( as, *sourcedp.ptr() ) : false;
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    const RegularSeisDataPack& cachedp )
{
    return setAttribData_( as, cachedp );
}


bool Engine::setAttribData_( const Attrib::SelSpec& as,
			     const SeisDataPack& cacheidin )
{
    ConstRefMan<SeisDataPack> seisdp = &cacheidin;
    const int idx = getCacheIndexOf( as );
    if ( attribcachespecs_.validIdx(idx) )
    {
	attribcachespecs_.get( idx )->seisdp_ = seisdp.getNonConstPtr();
    }
    else if ( seisdp )
    {
	auto* attribcachespec = as.is2D()
			      ? new CacheSpecs( as, activeGeomID() )
			      : new CacheSpecs( as, Survey::default3DGeomID() );
	attribcachespec->seisdp_ = seisdp.getNonConstPtr();
	attribcachespecs_.add( attribcachespec );
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as,
			    const TrcKeyZSampling& cs )
{
    ConstRefMan<SeisDataPack> seisdp = getAttribCacheDP( as );
    if ( !seisdp )
	return false;

    mDynamicCastGet(const RegularSeisDataPack*,regseisdp,seisdp.ptr());
    if ( !regseisdp )
	return false;

    TrcKeyZSampling cachedtkzs = regseisdp->sampling();
    const float zrgeps = 0.01f * SI().zStep();
    cachedtkzs.zsamp_.widen( zrgeps );
    return cachedtkzs.includes( cs );
}


void Engine::swapCacheAndItsBackup()
{
    const ObjectSet<CacheSpecs> tempcachespecs = attribcachespecs_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbackupcachespecs_ = tempcachespecs;
}


RefMan<FlatDataPack> Engine::getSeedPosDataPack( const TrcKey& tk, float z,
				    int nrtrcs, const ZSampling& zintv ) const
{
    TypeSet<Attrib::SelSpec> specs;
    getNeededAttribs( specs );
    if ( specs.isEmpty() )
	return nullptr;

    ConstRefMan<SeisDataPack> seisdp = getAttribCacheDP( specs.first() );
    if ( !seisdp )
	return nullptr;

    const int globidx = seisdp->getNearestGlobalIdx( tk );
    if ( globidx < 0 )
	return nullptr;

    ZSampling zintv2 = zintv; zintv2.step_ = seisdp->zRange().step_;
    const int nrz = zintv2.nrSteps() + 1;
    auto* seeddata = new Array2DImpl<float>( nrtrcs, nrz );
    if ( !seeddata->isOK() )
    {
	delete seeddata;
	return nullptr;
    }

    seeddata->setAll( mUdf(float) );

    const int trcidx0 = globidx - (int)(nrtrcs/2);
    const ZSampling zsamp = seisdp->zRange();
    const int zidx0 = zsamp.getIndex( z + zintv.start_ );
    for ( int tidx=0; tidx<nrtrcs; tidx++ )
    {
	const int curtrcidx = trcidx0+tidx;
	if ( curtrcidx<0 || curtrcidx >= seisdp->nrTrcs() )
	    continue;

	const OffsetValueSeries<float> ovs =
			    seisdp->getTrcStorage( 0, trcidx0+tidx );
	for ( int zidx=0; zidx<nrz; zidx++ )
	{
	    const float val = ovs[zidx0+zidx];
	    seeddata->set( tidx, zidx, val );
	}
    }

    StepInterval<double> trcrg;
    trcrg.start_ = tk.trcNr() - (nrtrcs)/2;
    trcrg.stop_ = tk.trcNr() + (nrtrcs)/2;
    StepInterval<double> zrg;
    zrg.start_ = mCast(double,zsamp.atIndex(zidx0));
    zrg.stop_ = mCast(double,zsamp.atIndex(zidx0+nrz-1));
    zrg.step_ = mCast(double,zsamp.step_);

    RefMan<FlatDataPack> fdp = new FlatDataPack( "Seismics", seeddata );
    fdp->posData().setRange( true, trcrg );
    fdp->posData().setRange( false, zrg );
    return fdp;
}


const char* Engine::errMsg() const
{
    return errmsg_.str();
}


BufferString Engine::setupFileName( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj ? EM::Surface::getSetupFileName(*ioobj) : BufferString();
}


void Engine::fillPar( IOPar& iopar ) const
{
    int trackeridx = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	ConstRefMan<EMTracker> tracker = trackers_[idx];
	if ( !tracker )
	    continue;

	IOPar localpar;
	localpar.set( sKeyObjectID(),EM::EMM().getMultiID(tracker->objectID()));
	localpar.setYN( sKeyEnabled(), tracker->isEnabled() );

	EMSeedPicker* seedpicker =
			    tracker.getNonConstPtr()->getSeedPicker(false);
	if ( seedpicker )
	    localpar.set( sKeySeedConMode(), seedpicker->getTrackMode() );

//	tracker->fillPar( localpar );

	iopar.mergeComp( localpar, toString(trackeridx) );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume_.fillPar( iopar );
}


bool Engine::usePar( const IOPar& par )
{
    cleanup();

    TrcKeyZSampling newvolume;
    if ( newvolume.usePar(par) )
	setActiveVolume( newvolume );

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    par.get( sKeyNrTrackers(), nrtrackers );

    bool haserrors = false;
    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	PtrMan<IOPar> localpar = par.subselect( idx );
	MultiID midtoload;
	if ( !localpar || !localpar->get(sKeyObjectID(),midtoload) ||
	     midtoload.isUdf() )
	{
	    haserrors = true;
	    continue;
	}

	trackerpars_.add( localpar.release() );
	trackermids_ += midtoload;
    }

    return !haserrors;
}


bool Engine::needRestoredTracker( const MultiID& mid ) const
{
    for ( const auto& storedid : trackermids_ )
	if ( storedid == mid )
	    return true;

    return false;
}


bool Engine::restoreTracker( const EM::ObjectID& emid )
{
    RefMan<EMTracker> tracker = getTrackerByID( emid );
    if ( !tracker )
	return false;

    ConstRefMan<EM::EMObject> emobj = tracker->emObject();
    if ( !emobj )
	return false;

    const MultiID& trackerobjmid = emobj->multiID();
    for ( int idx=0; idx<trackermids_.size(); idx++ )
    {
	if ( trackermids_[idx] != trackerobjmid || !trackerpars_.validIdx(idx) )
	    continue;

	const IOPar& par = *trackerpars_.get( idx );
	bool doenable = true;
	par.getYN( sKeyEnabled(), doenable );
	tracker->enable( doenable );

	int seedconmode = -1;
	par.get( sKeySeedConMode(), seedconmode );
	EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( seedpicker && seedconmode!=-1 )
	    seedpicker->setTrackMode( (EMSeedPicker::TrackMode)seedconmode );

	// old restore session policy without separate tracking setup file
	tracker->usePar( par );
	return true;
    }

    return false;
}


// Engine::CacheSpecs

Engine::CacheSpecs::CacheSpecs( const Attrib::SelSpec& as,
				const Pos::GeomID& geomid )
    : attrsel_(as)
    , geomid_(geomid)
{}


Engine::CacheSpecs::~CacheSpecs()
{}

} // namespace MPE
