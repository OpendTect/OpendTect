/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2005
________________________________________________________________________

-*/

#include "horizon2dseedpicker.h"

#include "attribdataholder.h"
#include "attribstorprovider.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seisinfo.h"
#include "horizon2dextender.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "sorting.h"
#include "posinfo2dsurv.h"
#include "survgeom2d.h"


namespace MPE
{

Horizon2DSeedPicker::Horizon2DSeedPicker( EMTracker& tracker )
    : EMSeedPicker(tracker)
    , geomid_(Survey::GeometryManager::cUndefGeomID())
{
}


Horizon2DSeedPicker::~Horizon2DSeedPicker()
{}


void Horizon2DSeedPicker::setLine( Pos::GeomID geomid )
{
    geomid_ = geomid;
}


bool Horizon2DSeedPicker::canAddSeed( const Attrib::SelSpec& as )
{
    if ( trackmode_ == DrawBetweenSeeds )
	return true;

    seedToBeAddedRemoved.trigger();
    TypeSet<Attrib::SelSpec> neededattribs;
    tracker_.getNeededAttribs( neededattribs );
    for ( int idx=0; idx<neededattribs.size(); idx++ )
    {
	if ( neededattribs[idx].id().asInt()==as.id().asInt() &&
	     neededattribs[idx].isStored()==as.id().isStored() )
	    return true;
    }
    return false;
}


#define mGetHorizon(hor,escval) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet(EM::Horizon2D*,hor,EM::EMM().getObject(emobjid)) \
    if ( !hor ) return escval;


bool Horizon2DSeedPicker::startSeedPick()
{
    if ( !EMSeedPicker::startSeedPick() )
	return false;

    mGetHorizon(hor,false);

    if ( geomid_ == Survey::GeometryManager::cUndefGeomID() )
	return false;

    EM::Horizon2DGeometry& geom = hor->geometry();
    if ( sowermode_ )
	return true;

    for ( int idx=0; idx<geom.nrLines(); idx++ )
    {
	if ( geomid_ == geom.geomID(idx) )
	    return true;
    }

    const int oldnrlines = geom.nrLines();
    if ( geom.includeLine(geomid_) && oldnrlines==geom.nrLines() )
    {
	const TypeSet<EM::PosID>& pids =
			    *hor->getPosAttribList(EM::EMObject::sSeedNode() );

	for ( int idx=pids.size()-1; idx>=0; idx-- )
	{
	    if ( Coord( hor->getPos(pids[idx]) ).isDefined() )
		continue;

	    hor->setPosAttrib(pids[idx],EM::EMObject::sSeedNode(),false,false);
	}
    }

    return true;
}


#define mGetHorAndColrg(hor,colrg,escval) \
    mGetHorizon(hor,escval); \
    if ( sectionid_<0 || geomid_==Survey::GeometryManager::cUndefGeomID() ) \
	return escval; \
    const StepInterval<int> colrg = \
	hor->geometry().colRange( sectionid_, geomid_ );


bool Horizon2DSeedPicker::addSeed( const TrcKeyValue& seed, bool drop,
				   const TrcKeyValue& seedkey )
{
    addedseed_ = seed;
    if ( !sowermode_ )
	seedToBeAddedRemoved.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorizon(hor2d,false);
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return false;

    seedlist_.erase();
    seedlist_ += seed.tk_;

    bool res = true;
    if ( sowermode_ )
    {
	// Duplicate promotes hidden seed to visual seed in sower mode
	const bool isvisualseed = lastseed_==seed;

	if ( isvisualseed || lastseed_!=seed )
	{
	    hor2d->setZAndNodeSourceType(
		seed.tk_, seed.val_, true, EM::EMObject::Auto );
	    hor2d->setAttrib( seed.tk_, EM::EMObject::sSeedNode(),
			      isvisualseed, true );
	    if ( trackmode_ != DrawBetweenSeeds )
		tracker_.snapPositions( seedlist_ );

	    seedlist_ += lastseed_.tk_;
	    interpolateSeeds( false );
	}
    }
    else
    {
	const bool pickedposwasdef = hor2d->hasZ( seed.tk_ );
	if ( !drop || !pickedposwasdef )
	{
	    hor2d->setZAndNodeSourceType(
		seed.tk_, seed.val_, true, EM::EMObject::Auto );
	    if ( trackmode_ != DrawBetweenSeeds )
		tracker_.snapPositions( seedlist_ );

	    addedseed_.val_ = hor2d->getZ( seed.tk_ );
	    seedAdded.trigger();
	}

	hor2d->setAttrib( seed.tk_, EM::EMObject::sSeedNode(), true, true );

	res = drop ? true
		   : retrackOnActiveLine( seed.tk_.trcNr(), pickedposwasdef );
    }

    lastseed_ = seed;
    return res;
}


int Horizon2DSeedPicker::nrLineNeighbors( int colnr ) const
{
    mGetHorAndColrg(hor,colrg,-1);
    int col = 0;
    int nrneighbors = 0;

    for ( int idx=-1; idx<=1; idx+=2 )
    {
	col = colnr;
	while ( col>colrg.start && col<colrg.stop )
	{
	    col += idx*colrg.step;
	    const Coord3 pos = hor->getPos( sectionid_, geomid_, col );
	    if ( Coord(pos).isDefined() )
	    {
		if ( pos.isDefined() )
		    nrneighbors++;
		break;
	    }
	}
    }

    return nrneighbors;
}


bool Horizon2DSeedPicker::updatePatchLine( bool doerase )
{
    if ( trackmode_ == TrackFromSeeds && !doerase )
	return addPatchSowingSeeds();

    if ( trackmode_ != DrawBetweenSeeds &&
	trackmode_ != DrawAndSnap && !doerase )
	return false;

    const TypeSet<TrcKeyValue>& path = patch_->getPath();
    mGetHorizon( hor2d, false )

    hor2d->setBurstAlert( true );
    for ( int idx=0; idx<path.size(); idx++ )
    {
	const float val = !doerase ? path[idx].val_ : mUdf(float);
	hor2d->setZAndNodeSourceType( path[idx].tk_, val, true,
	    EM::EMObject::Manual );
	if ( trackmode_ == DrawAndSnap )
	{
	    hor2d->setAttrib( path[idx].tk_, EM::EMObject::sSeedNode(),
		false, true );
	    TypeSet<TrcKey> seed;
	    seed += path[idx].tk_;
	    tracker_.snapPositions( seed );
	}
    }

    seedlist_.erase();
    for ( int idx=0; idx<path.size(); idx++ )
    {
	if ( path[idx].tk_.isUdf() )
	    continue;
	seedlist_ += path[idx].tk_;
    }
    interpolateSeeds( true );
    hor2d->setBurstAlert( false );
    EM::EMM().undo(hor2d->id()).setUserInteractionEnd(
	EM::EMM().undo(hor2d->id()).currentEventID() );
    return true;
}



bool Horizon2DSeedPicker::addPatchSowingSeeds()
{
    mGetHorizon( hor2d, false )
    hor2d->setBurstAlert(true);

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
    hor2d->setBurstAlert( false );
    EM::EMM().undo(hor2d->id()).setUserInteractionEnd(
	EM::EMM().undo(hor2d->id()).currentEventID() );
    return true;
}


bool Horizon2DSeedPicker::removeSeed( const TrcKey& tk, bool environment,
				      bool retrack )
{
    seedToBeAddedRemoved.trigger();

    if ( blockpicking_ )
	return true;

    if ( tk.geomID() != geomid_ )
	return false;

    mGetHorizon(hor,false);
    hor->setAttrib( tk, EM::EMObject::sSeedNode(), false, true );
    if ( environment || !nrLineNeighbors(tk.trcNr()) )
	hor->setZAndNodeSourceType( tk, mUdf(float), true, EM::EMObject::Auto );

    seedlist_.erase();
    seedRemoved.trigger();
    return
	environment ? retrackOnActiveLine(tk.trcNr(),true,!retrack) : true;
}


bool Horizon2DSeedPicker::getNextSeedPos( int seedpos, int dirstep,
					  int& nextseedpos ) const
{
    mGetHorAndColrg(hor,colrg,false);
    RowCol currc( hor->geometry().lineIndex(geomid_), seedpos );
    while ( true )
    {
	currc.col() += dirstep;
	if ( !colrg.includes(currc.col(),false) )
	    return false;
	const EM::PosID pid( hor->id(), sectionid_, currc.toInt64() );
	if ( hor->isPosAttrib( pid, EM::EMObject::sSeedNode() ) )
	{
	    nextseedpos = currc.col();
	    return true;
	}
    }

    return false;
}


TrcKey Horizon2DSeedPicker::replaceSeed( const TrcKey& oldseed,
					 const TrcKeyValue& newseedin )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d || trackmode_ != DrawBetweenSeeds )
	return TrcKey::udf();

    sowermode_ = false;

    mGetHorAndColrg(hor2d,colrg,TrcKey::udf());
    hor2d->setBurstAlert( true );
    int prevseedpos = mUdf(int);
    int nextseedpos = mUdf(int);
    getNextSeedPos( oldseed.trcNr(), -colrg.step, prevseedpos );
    getNextSeedPos( oldseed.trcNr(), colrg.step, nextseedpos );
    if ( mIsUdf(prevseedpos) && mIsUdf(nextseedpos) )
    {
	hor2d->setBurstAlert( false );
	return TrcKey::udf();
    }

    TrcKeyValue newseed = newseedin;
    TrcKey& newtk = newseed.tk_;
    if ( !mIsUdf(prevseedpos) && newtk.trcNr()<=prevseedpos )
	newtk.trcNr() = prevseedpos + colrg.step;
    else if ( !mIsUdf(nextseedpos) && newtk.trcNr()>=nextseedpos )
	newtk.trcNr() = nextseedpos - colrg.step;

    removeSeed( oldseed, true, false );
    addSeed( newseed, false, newseed );
    hor2d->setBurstAlert( false );
    return newseed.tk_;
}


bool Horizon2DSeedPicker::retrackOnActiveLine( int startcol,
					       bool startwasdefined,
					       bool eraseonly )
{
    mGetHorAndColrg(hor2d,colrg,false);

    trackbounds_.erase();
    junctions_.erase();

    if ( colrg.includes(startcol,false) )
    {
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      -colrg.step );
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      colrg.step );
    }
    else
    {
	// traverse whole active line
	extendSeedListEraseInBetween( true, colrg.start, false, -colrg.step );
	extendSeedListEraseInBetween( true, colrg.start-colrg.step, false,
				      colrg.step );

	if ( trackmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }

    bool res = true;

    if ( eraseonly )
	return true;

    res = retrackFromSeedList();

    return res;
}


void Horizon2DSeedPicker::extendSeedListEraseInBetween(
					    bool wholeline, int startcol,
					    bool startwasdefined, int step )
{
    mGetHorAndColrg(hor2d,colrg, );
    eraselist_.erase();

    TrcKey curtk( geomid_, startcol );

    bool seedwasadded = hor2d->hasZ( curtk ) && !wholeline;
    bool curdefined = startwasdefined;

    while ( true )
    {
	const TrcKey prevtk = curtk;
	const bool prevdefined = curdefined;

	curtk.trcNr() += step;

	// reaching end of line
	if ( !colrg.includes(curtk.trcNr(),false) )
	{
	    if	( trackmode_ == TrackFromSeeds )
		trackbounds_ += prevtk;

	    if ( seedwasadded && trackmode_!=TrackFromSeeds )
		return;

	    break;
	}

	if ( !hor2d->hasZ(curtk) )
	    continue;

	curdefined = true;

	// running into a seed point
	if ( hor2d->isAttrib(curtk,EM::EMObject::sSeedNode()) )
	{
	    const bool onewaytracking = seedwasadded && !prevdefined &&
					trackmode_==TrackFromSeeds;
	    if ( onewaytracking )
		trackbounds_ += curtk;
	    else
		seedlist_ += curtk;

	    if ( wholeline )
		continue;

	    break;
	}

	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    if	( trackmode_==TrackFromSeeds )
		trackbounds_ += curtk;
	    else
		seedlist_ += curtk;

	    junctions_ += prevtk;
	    junctions_ += curtk;
	    break;
	}

	// to erase points attached to start
	if ( curdefined &&
	    !hor2d->isNodeSourceType(curtk,EM::EMObject::Manual) )
	    eraselist_ += curtk;
    }

    for ( int idx=0; idx<eraselist_.size(); idx++ )
	hor2d->setZAndNodeSourceType(
	eraselist_[idx], mUdf(float), true, EM::EMObject::Auto );
}


bool Horizon2DSeedPicker::retrackFromSeedList()
{
    if ( seedlist_.isEmpty() )
	return true;
    if ( blockpicking_ )
	return true;
    if ( trackmode_ == DrawBetweenSeeds )
	return interpolateSeeds( false );

    mGetHorizon(hor,false);

    SectionTracker* sectracker = tracker_.getSectionTracker( sectionid_, true );
    SectionExtender* extender = sectracker->extender();
    mDynamicCastGet( HorizonAdjuster*, adjuster, sectracker->adjuster() );
    mDynamicCastGet( Horizon2DExtender*, extender2d, extender );
    if ( !extender2d )
	return false;

    extender->setDirection( TrcKeyValue(TrcKey(0,0)) );
    extender->setExtBoundary( getTrackBox() );
    extender2d->setGeomID( geomid_ );

    TypeSet<TrcKey> addedpos;
    TypeSet<TrcKey> addedpossrc;

    for ( int idx=0; idx<seedlist_.size(); idx++ )
	addedpos += seedlist_[idx];

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
	    if ( !hor->hasZ(addedpos[idx]) )
		addedpos.removeSingle(idx);
	}
    }

    extender->unsetExtBoundary();

    return true;
}


bool Horizon2DSeedPicker::reTrack()
{
    seedlist_.erase();
    const bool res = retrackOnActiveLine( Values::Undef<int>::val() , false );
    return res;
}


bool Horizon2DSeedPicker::interpolateSeeds()
{
    return interpolateSeeds( false );
}


bool Horizon2DSeedPicker::interpolateSeeds( bool manualnode )
{
    mGetHorAndColrg(hor,colrg,false);

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 )
	return true;

    TypeSet<int> sortval;
    TypeSet<int> sortidx;
    for ( int idx=0; idx<nrseeds; idx++ )
    {
	if ( seedlist_[idx].geomID() != geomid_ )
	    continue;

	sortval += seedlist_[idx].trcNr();
	sortidx += idx;
    }

    const int nrvalidseeds = sortval.size();
    sort_coupled( sortval.arr(), sortidx.arr(), nrvalidseeds );

    TypeSet<TrcKey> snaplist;
    TrcKey tk( geomid_, -1 );
    for ( int vtx=0; vtx<nrvalidseeds-1; vtx++ )
    {
	const Coord3 startpos = hor->getCoord( seedlist_[ sortidx[vtx] ] );
	const Coord3 endpos = hor->getCoord( seedlist_[ sortidx[vtx+1] ] );

	double totarclen = 0.0;
	Coord prevpos = startpos;
	tk.trcNr() = sortval[vtx];
	while ( tk.trcNr()<sortval[vtx+1] )
	{
	    tk.trcNr() += colrg.step;
	    const Coord curpos = hor->getCoord( tk );
	    if ( !curpos.isDefined() )
		continue;

	    totarclen += prevpos.distTo( curpos );
	    prevpos = curpos;
	}

	double arclen = 0.0;
	prevpos = startpos;
	tk.trcNr() = sortval[vtx] + colrg.step;
	for ( ; tk.trcNr()<sortval[vtx+1]; tk.trcNr() += colrg.step  )
	{
	    const Coord curpos = hor->getCoord( tk );
	    if ( !curpos.isDefined() )
		continue;

	    arclen += prevpos.distTo( curpos );
	    prevpos = curpos;

	    const double frac = arclen / totarclen;
	    const double curz = (1-frac) * startpos.z + frac * endpos.z;
	    const EM::EMObject::NodeSourceType type = manualnode ?
		EM::EMObject::Manual : EM::EMObject::Auto;
	    hor->setZAndNodeSourceType( tk, (float)curz, true, type );
	    hor->setAttrib( tk, EM::EMObject::sSeedNode(), false, true );

	    if ( trackmode_ != DrawBetweenSeeds )
		snaplist += tk;
	}
    }

    tracker_.snapPositions( snaplist );
    return true;
}


TrcKeyZSampling Horizon2DSeedPicker::getTrackBox() const
{
    TrcKeyZSampling trackbox( true );
    trackbox.hsamp_.init( false );

    for ( int idx=0; idx<seedlist_.size(); idx++ )
	trackbox.hsamp_.include( seedlist_[idx] );
    for ( int idx=0; idx<trackbounds_.size(); idx++ )
	trackbox.hsamp_.include( trackbounds_[idx] );

    return trackbox;
}

} // namespace MPE
