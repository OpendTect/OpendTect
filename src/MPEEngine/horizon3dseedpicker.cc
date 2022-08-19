/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon3dseedpicker.h"

#include "autotracker.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "executor.h"
#include "faulttrace.h"
#include "mpeengine.h"
#include "horizonadjuster.h"
#include "randomlinegeom.h"
#include "sectionextender.h"
#include "sectiontracker.h"
#include "sorting.h"
#include "survinfo.h"
#include "trckeyvalue.h"
#include "emmanager.h"
#include "undo.h"


namespace MPE
{

Horizon3DSeedPicker::Horizon3DSeedPicker( EMTracker& tracker )
    : EMSeedPicker(tracker)
    , fltdataprov_(0)
{
}


Horizon3DSeedPicker::~Horizon3DSeedPicker()
{}


#define mGetHorizon(hor,retval) \
    mDynamicCastGet(EM::Horizon3D*,hor,tracker_.emObject())\
    if ( !hor ) return retval;

bool Horizon3DSeedPicker::addSeed( const TrcKeyValue& seed, bool drop,
				   const TrcKeyValue& seedkey )
{
    addedseed_ = seed;
    if ( !sowermode_ )
	seedToBeAddedRemoved.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorizon( hor3d, false )

    const TrcKeySampling hrg = engine().activeVolume().hsamp_;
    if ( hrg.includes(seed.tk_,true) && !hrg.includes(seed.tk_,false) )
	addedseed_.tk_ = hrg.getNearest( seed.tk_ );

    const StepInterval<float> zrg = engine().activeVolume().zsamp_;
    if ( !zrg.includes(addedseed_.val_,false) || !hrg.includes(addedseed_.tk_) )
	return false;

    if ( fltdataprov_ && hrg.includes(lastsowseed_.tk_) )
    {
	if ( sowermode_ &&
	     fltdataprov_->isCrossingFault(addedseed_.tk_.position(),
					   addedseed_.val_,
					   lastsowseed_.tk_.position(),
					   lastsowseed_.val_) )
	{
	    lastseed_ = addedseed_;
	    return false;
	}
    }

    bool res = true;
    if ( sowermode_ )
    {
	lastsowseed_ = addedseed_;
	// Duplicate promotes hidden seed to visual seed in sower mode
	const bool isvisualseed = lastseed_==addedseed_;

	if ( isvisualseed || lastseed_!=addedseed_ )
	{
	    hor3d->setZAndNodeSourceType(
		addedseed_.tk_, addedseed_.val_, true, EM::EMObject::Auto );
	    hor3d->setAttrib( addedseed_.tk_, EM::EMObject::sSeedNode(),
			      isvisualseed, true );
	    seedlist_.erase();
	    seedlist_ += addedseed_.tk_;
	    if ( trackmode_ != DrawBetweenSeeds )
		tracker_.snapPositions( seedlist_ );

	    seedlist_ += lastseed_.tk_;
	    interpolateSeeds( false );
	}
    }
    else
    {
	lastsowseed_ = TrcKeyValue::udf();
	propagatelist_.erase();
	propagatelist_ += addedseed_.tk_;

	const bool pickedposwasdef = hor3d->hasZ( addedseed_.tk_ );
	if ( !drop || !pickedposwasdef )
	{
	    hor3d->setZAndNodeSourceType(
		addedseed_.tk_, addedseed_.val_, true, EM::EMObject::Auto );
	    if ( trackmode_ != DrawBetweenSeeds )
		tracker_.snapPositions( propagatelist_ );

	    addedseed_.val_ = hor3d->getZ( addedseed_.tk_ );
	}

	hor3d->setAttrib( addedseed_.tk_, EM::EMObject::sSeedNode(),
			  true, true );
	if ( !drop || !pickedposwasdef )
	    seedAdded.trigger();

	// Loop may add new items to the tail of the propagate list.
	for ( int idx=0; idx<propagatelist_.size() && !drop; idx++ )
	{
	    seedlist_.erase();
	    seedlist_ += propagatelist_[idx];

	    const bool startwasdef = idx ? true : pickedposwasdef;
	    res = retrackOnActiveLine( propagatelist_[idx], startwasdef )
			&& res;
	}
    }

    lastseed_ = addedseed_;
    return res;
}


bool Horizon3DSeedPicker::removeSeed( const TrcKey& seed, bool environment,
				      bool retrack )
{
    mGetHorizon( hor3d, false );

    if ( !hor3d || hor3d->isNodeLocked(seed) )
	return false;

    seedToBeAddedRemoved.trigger();

    if ( blockpicking_ )
	return true;

    hor3d->setAttrib( seed, EM::EMObject::sSeedNode(), false, true );

    if ( environment ||
	 nrLineNeighbors(
		seed.position())+nrLateralNeighbors(seed.position())==0 )
	hor3d->setZAndNodeSourceType(
	seed, mUdf(float), true, EM::EMObject::Auto );

    int res = true;
    if ( environment )
    {
	propagatelist_.erase(); seedlist_.erase();
	res = retrackOnActiveLine( seed, true, !retrack );
    }

    return res;
}


bool Horizon3DSeedPicker::getNextSeed( const BinID& seed,
				       const BinID& dir,
				       BinID& nextseed ) const
{
    mGetHorizon( hor3d, false );
    BinID bid = seed;
    while ( true )
    {
	bid += dir;
	if ( !engine().activeVolume().hsamp_.includes(bid) )
	    return false;

	if ( hor3d->isAttrib(TrcKey(bid),EM::EMObject::sSeedNode()) )
	{
	    nextseed = bid;
	    return true;
	}
    }

    return false;
}


TrcKey Horizon3DSeedPicker::replaceSeed( const TrcKey& oldseed,
					 const TrcKeyValue& newseedin )
{
    mGetHorizon( hor3d, TrcKey::udf() );
    if ( trackmode_ != DrawBetweenSeeds )
	return TrcKey::udf();

    sowermode_ = false;

    BinID dir;
    if ( !lineTrackDirection(dir) )
	return TrcKey::udf(); //TODO implement for RandomLine

    hor3d->setBurstAlert( true );
    BinID prevseed=BinID::udf(), nextseed=BinID::udf();
    getNextSeed( oldseed.position(), -dir, prevseed );
    getNextSeed( oldseed.position(), dir, nextseed );
    if ( prevseed.isUdf() && nextseed.isUdf() )
    {
	hor3d->setBurstAlert( false );
	return TrcKey::udf();
    }

    TrcKeyValue newseed = newseedin;
    TrcKey& newtk = newseed.tk_;
    const TrcKey& cstnewtk = const_cast<const TrcKey&>( newtk );
    const bool inltracking = dir.lineNr()==0;
    const int dirlength = inltracking ? dir.crl() : dir.inl();
    int adjustednewpos = inltracking ? cstnewtk.crl() : cstnewtk.crl();
    const int prevseedpos = inltracking ? prevseed.crl() : prevseed.inl();
    const int nextseedpos = inltracking ? nextseed.crl() : nextseed.inl();
    if ( !mIsUdf(prevseedpos) && adjustednewpos<=prevseedpos )
	adjustednewpos = prevseedpos + dirlength;
    else if ( !mIsUdf(nextseedpos) && adjustednewpos>=nextseedpos )
	adjustednewpos = nextseedpos - dirlength;
    if ( inltracking )
	newtk.setCrl( adjustednewpos );
    else
	newtk.setInl( adjustednewpos );

    removeSeed( oldseed, true, false );
    addSeed( newseed, false, newseed );
    hor3d->setBurstAlert( false );
    return newseed.tk_;
}


bool Horizon3DSeedPicker::reTrack()
{
    propagatelist_.erase(); seedlist_.erase();

    const bool res = retrackOnActiveLine( TrcKey(BinID(-1,-1)), false );
    return res;
}


bool Horizon3DSeedPicker::retrackOnActiveLine( const TrcKey& starttk,
					     bool startwasdefined,
					     bool eraseonly )
{

    trackbounds_.erase();
    junctions_.erase();

    BinID dir;
    if ( !lineTrackDirection(dir) )
	return retrackFromSeedList(); // track on Rdl

    if ( engine().activeVolume().hsamp_.includes(starttk) )
    {
	extendSeedListEraseInBetween( false, starttk, startwasdefined, -dir );
	extendSeedListEraseInBetween( false, starttk, startwasdefined, dir );
    }
    else
    {
	// traverse whole active line
	const TrcKey dummystart = engine().activeVolume().hsamp_.trcKeyAt(0);
	extendSeedListEraseInBetween( true, dummystart, false, -dir );
	extendSeedListEraseInBetween( true, dummystart, false, dir );

	if ( trackmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }

    if ( eraseonly )
	return true;

    const bool res = retrackFromSeedList();
    processJunctions();

    return res;
}


void Horizon3DSeedPicker::processJunctions()
{
    mGetHorizon( hor3d, );

    for ( int idx=0; idx<junctions_.size(); idx+=3 )
    {
	const TrcKey& prevtk = junctions_[idx];
	if ( !hor3d->hasZ(prevtk) )
	    continue;

	const TrcKey& curtk = junctions_[idx+1];
	//emobj->setPosAttrib( curpid, EM::EMObject::sSeedNode(), true );

	const TrcKey& nexttk = junctions_[idx+2];
	if ( trackmode_!=TrackFromSeeds || hor3d->hasZ(nexttk) )
	    continue;

	const float oldz = hor3d->getZ( curtk );
	TypeSet<TrcKey> curtklisted;
	curtklisted += curtk;
	tracker_.snapPositions( curtklisted );
	const float snappedoldz = hor3d->getZ( curtk );

	hor3d->setZAndNodeSourceType(
	    curtk, hor3d->getZ(prevtk), true, EM::EMObject::Auto );
	tracker_.snapPositions( curtklisted );
	const float snappednewz = hor3d->getZ( curtk );

	if ( mIsEqual(snappednewz,snappedoldz,mDefEps) &&
		!propagatelist_.isPresent(curtk) )
	    propagatelist_ += curtk;

	hor3d->setZAndNodeSourceType(
	    curtk, oldz, true, EM::EMObject::Auto );
    }
}


#define mStoreSeed( onewaytracking ) \
    if ( onewaytracking ) \
	trackbounds_ += curtk; \
    else \
    { \
	seedlist_ += curtk; \
    }

#define mStoreJunction() \
{ \
    junctions_ += prevtk; \
    junctions_ += curtk; \
    const TrcKey nexttk( curtk.position() + dir ); \
    junctions_ += nexttk; \
}


bool Horizon3DSeedPicker::updatePatchLine( bool doerase )
{
    if ( trackmode_ == TrackFromSeeds && !doerase )
	return addPatchSowingSeeds();

    if ( trackmode_ != DrawBetweenSeeds &&
	trackmode_ != DrawAndSnap && !doerase )
	return false;

    const TrcKeySampling hrg = engine().activeVolume().hsamp_;
    const StepInterval<float> zrg = engine().activeVolume().zsamp_;
    TypeSet<TrcKeyValue> path = patch_->getPath();
    mGetHorizon( hor3d, false )

    seedlist_.erase();
    hor3d->setBurstAlert( true );
    hor3d->geometryElement()->blockCallBacks( true, false );

    for ( int idx=0; idx<patch_->nrSeeds(); idx++ )
    {
	const float val = !doerase ? path[idx].val_ : mUdf(float);
	hor3d->setZAndNodeSourceType( path[idx].tk_, val, true,
	    EM::EMObject::Manual );
	if ( trackmode_ == DrawAndSnap )
	{
	    hor3d->setAttrib( path[idx].tk_, EM::EMObject::sSeedNode(),
		false, true );
	    TypeSet<TrcKey> seed;
	    seed += path[idx].tk_;
	    tracker_.snapPositions( seed );
	}

	if ( path[idx].tk_.isUdf() ||
	    !zrg.includes(path[idx].val_,false) ||
	    !hrg.includes(path[idx].tk_) )
	continue;
	seedlist_ += path[idx].tk_;
    }

    interpolateSeeds( true );
    hor3d->geometryElement()->blockCallBacks( false, true );
    hor3d->setBurstAlert( false );
    EM::EMM().undo(hor3d->id()).setUserInteractionEnd(
	EM::EMM().undo(hor3d->id()).currentEventID() );
    return true;
}


bool Horizon3DSeedPicker::addPatchSowingSeeds()
{
    mGetHorizon( hor3d, false )
    hor3d->setBurstAlert( true );
    hor3d->geometryElement()->blockCallBacks(true,false);

    const TypeSet<TrcKeyValue>& path = patch_->getPath();
    int firstthreebendpoints_ = 0;
    for ( int idx=0; idx<patch_->nrSeeds(); idx++ )
    {
	// for the first three seeds they are always are seeds
	sowermode_ = firstthreebendpoints_>2 ? true : false;
	const TrcKeyValue& seed = path[idx];
	addSeed( seed, false, seed );
	firstthreebendpoints_++;
    }

    hor3d->geometryElement()->blockCallBacks(false,true);
    hor3d->setBurstAlert( false );

    EM::EMM().undo(hor3d->id()).setUserInteractionEnd(
	EM::EMM().undo(hor3d->id()).currentEventID() );
    return true;
}


void Horizon3DSeedPicker::extendSeedListEraseInBetween(
				bool wholeline, const TrcKey& starttk,
				bool startwasdefined, const BinID& dir )
{
    eraselist_.erase();

    mGetHorizon( hor3d, );
    const bool seedwasadded = hor3d->hasZ( starttk ) && !wholeline;

    TrcKey curtk = starttk;
    bool curdefined = startwasdefined;

    while ( true )
    {
	const TrcKey prevtk = curtk;
	const bool prevdefined = curdefined;

	curtk.setPosition( curtk.position() + dir );

	// reaching end of survey
	if ( !engine().activeVolume().hsamp_.includes(curtk) )
	{
	    if ( trackmode_==TrackFromSeeds )
		trackbounds_ += prevtk;

	    if ( seedwasadded && trackmode_!=TrackFromSeeds )
		return;

	    break;
	}

	curdefined = hor3d->hasZ( curtk );

	// running into a seed point
	if ( hor3d->isAttrib(curtk,EM::EMObject::sSeedNode()) )
	{
	    const bool onewaytracking = seedwasadded && !prevdefined &&
					trackmode_==TrackFromSeeds;
	    mStoreSeed( onewaytracking );

	    if ( onewaytracking )
		mStoreJunction();

	    if ( wholeline )
		continue;

	    break;
	}

	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    mStoreSeed( trackmode_==TrackFromSeeds );
	    mStoreJunction();
	    break;
	}

	// to erase points attached to start
	const EM::PosID pid( hor3d->id(),  curtk.position() );
	if ( curdefined &&
	    !hor3d->isNodeSourceType(pid,EM::EMObject::Manual) )
	    eraselist_ += curtk;
    }

    for ( int idx=0; idx<eraselist_.size(); idx++ )
	hor3d->setZAndNodeSourceType(
	eraselist_[idx], mUdf(float), true, EM::EMObject::Auto );
}


bool Horizon3DSeedPicker::retrackFromSeedList()
{
    if ( seedlist_.isEmpty() || blockpicking_ )
	return true;

    if ( trackmode_ == DrawBetweenSeeds )
	return interpolateSeeds( false );

    mGetHorizon( hor3d, false );

    SectionTracker* sectracker = tracker_.getSectionTracker( true );
    SectionExtender* extender = sectracker->extender();
    mDynamicCastGet(HorizonAdjuster*,adjuster,sectracker->adjuster());
    adjuster->setAttributeSel( 0, selspec_ );

    extender->setExtBoundary( getTrackBox() );
    BinID dir;
    if ( !lineTrackDirection(dir) )
	extender->setDirection( TrcKeyValue(BinID(1,1),mUdf(float)) );
    else if ( extender->getExtBoundary().defaultDir() == TrcKeyZSampling::Inl )
	extender->setDirection( TrcKeyValue(BinID(0,1),mUdf(float)) );
    else if ( extender->getExtBoundary().defaultDir() == TrcKeyZSampling::Crl )
	extender->setDirection( TrcKeyValue(BinID(1,0),mUdf(float)) );

    TypeSet<TrcKey> addedpos;
    TypeSet<TrcKey> addedpossrc;

    for ( int idx=0; idx<seedlist_.size(); idx++ )
	addedpos += seedlist_[idx];

    hor3d->setBurstAlert( true );
    while ( addedpos.size() )
    {
	extender->reset();
	extender->setStartPositions( addedpos );
	while ( extender->nextStep()>0 ) ;

	addedpos = extender->getAddedPositions();
	addedpossrc = extender->getAddedPositionsSource();

	adjuster->reset();
	adjuster->setPositions( addedpos, &addedpossrc );
	while ( adjuster->nextStep()>0 ) ;

	for ( int idx=addedpos.size()-1; idx>=0; idx-- )
	{
	    if ( !hor3d->hasZ(addedpos[idx]) )
		addedpos.removeSingle(idx);
	}
    }

    hor3d->setBurstAlert( false );
    extender->unsetExtBoundary();

    return true;
}


bool Horizon3DSeedPicker::doesModeUseVolume() const
{ return trackmode_==TrackFromSeeds; }


int Horizon3DSeedPicker::nrLateralNeighbors( const BinID& bid ) const
{
    return nrLineNeighbors( bid, true );
}


int Horizon3DSeedPicker::nrLineNeighbors( const BinID& bid,
					  bool perptotrackdir ) const
{
    mGetHorizon(hor3d,-1);

    TypeSet<EM::PosID> neighpid;
    EM::PosID pid( hor3d->id(), hor3d->sectionID(0), bid.toInt64() );
    hor3d->geometry().getConnectedPos( pid, &neighpid );

    BinID dir;
    if ( !lineTrackDirection(dir,perptotrackdir) )
	return -1;

    int total = 0;
    for ( int idx=0; idx<neighpid.size(); idx++ )
    {
	BinID neighbid = SI().transform( hor3d->getPos(neighpid[idx]) );
	if ( bid.isNeighborTo(neighbid,dir) )
	    total++;
    }
    return total;
}


bool Horizon3DSeedPicker::interpolateSeeds()
{
    return interpolateSeeds( false );
}


bool Horizon3DSeedPicker::interpolateSeeds( bool setmanualnode )
{
    mGetHorizon( hor3d, false )

    BinID dir;
    const TrcKeyPath* rdlpath = 0;
    RandomLineID rdlid;
    if ( !lineTrackDirection(dir) )
    {
	rdlpath = engine().activePath();
	rdlid = engine().activeRandomLineID();
    }

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 )
	return true;

    TrcKeyPath nodes;
    RefMan<Geometry::RandomLine> rlgeom = nullptr;
    if ( rdlpath && rdlid.isValid() )
    {
	rlgeom = Geometry::RLM().get( rdlid );
	if ( rlgeom )
	    rlgeom->allNodePositions( nodes );
    }

    mAllocVarLenArr( int, sortval, nrseeds );
    mAllocVarLenArr( int, sortidx, nrseeds );

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const TrcKey& seed = seedlist_[idx];
	if ( !nodes.isEmpty() )
	{
	    const int sortvalidx = Geometry::RandomLine::getNearestPathPosIdx(
						    nodes, *rdlpath, seed );
	    if ( sortvalidx<0 )
		continue;

	    sortval[idx] = sortvalidx;
	}
	else
	    sortval[idx] = dir.inl() ? seed.lineNr() : seed.trcNr();

	sortidx[idx] = idx;
    }

    sort_coupled( mVarLenArr(sortval), mVarLenArr(sortidx), nrseeds );

    TypeSet<TrcKey> snaplist;
    const int step = rdlpath ? 1 : dir.inl() ? dir.inl() : dir.crl();
    for ( int vtx=0; vtx<nrseeds-1; vtx++ )
    {
	const int diff = sortval[vtx+1] - sortval[vtx];
	if ( fltdataprov_ )
	{
	    const float z1 = hor3d->getZ( seedlist_[sortidx[vtx+1]] );
	    const float z2 = hor3d->getZ( seedlist_[sortidx[vtx]] );
	    const BinID seed1bid = seedlist_[sortidx[vtx+1]].position();
	    const BinID seed2bid = seedlist_[sortidx[vtx]].position();
	    if ( seed1bid!=seed2bid && (
		    fltdataprov_->isOnFault(seed1bid,z1,1.0f) ||
		    fltdataprov_->isOnFault(seed2bid,z2,1.0f) ||
		    fltdataprov_->isCrossingFault(seed1bid,z1,seed2bid,z2) ) )
		continue;
	}

	const Coord3 seed1 = hor3d->getCoord( seedlist_[sortidx[vtx]] );
	const Coord3 seed2 = hor3d->getCoord( seedlist_[sortidx[vtx+1]] );
	for ( int idx=step; idx<diff; idx+=step )
	{
	    TrcKey tk;
	    Coord3 interpos;
	    const double frac = (double) idx / diff;
	    if ( !nodes.isEmpty() )
	    {
		const int startidx =
		    Geometry::RandomLine::getNearestPathPosIdx(
			nodes, *rdlpath, seedlist_[sortidx[vtx]] );
		if ( startidx<0 )
		    continue;

		tk = (*rdlpath)[ startidx + idx ];
		const double interpz = (1-frac) * seed1.z + frac  * seed2.z;
		interpos = Coord3( SI().transform(tk.position()), interpz );
	    }
	    else
	    {
		interpos = (1-frac) * seed1 + frac  * seed2;
		tk = TrcKey( SI().transform( interpos ) );
	    }

	    if ( tk.isUdf() )
		continue;

	    const EM::EMObject::NodeSourceType type = setmanualnode ?
		EM::EMObject::Manual :
		EM::EMObject::Auto;
	    hor3d->setZAndNodeSourceType( tk, (float)interpos.z, true, type );
	    hor3d->setAttrib( tk, EM::EMObject::sSeedNode(), false, true );

	    if ( trackmode_ != DrawBetweenSeeds )
		snaplist += tk;
	}
    }

    tracker_.snapPositions( snaplist );
    return true;
}


TrcKeyZSampling Horizon3DSeedPicker::getTrackBox() const
{
    TrcKeyZSampling trackbox( true );
    trackbox.hsamp_.init( false );
    for ( int idx=0; idx<seedlist_.size(); idx++ )
    {
	const TrcKeyValue& seed = seedlist_[idx];
	if ( engine().activeVolume().hsamp_.includes(seed.tk_) )
	    trackbox.hsamp_.include( seed.tk_ );
    }

    for ( int idx=0; idx<trackbounds_.size(); idx++ )
	trackbox.hsamp_.include( trackbounds_[idx] );

    return trackbox;
}

} // namespace MPE
