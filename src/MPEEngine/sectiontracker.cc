/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "sectiontracker.h"

#include "attribsel.h"
#include "emobject.h"
#include "iopar.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "parametricsurface.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "survinfo.h"

namespace MPE
{

const char* SectionTracker::trackerstr = "Tracker";
const char* SectionTracker::useadjusterstr = "Use adjuster";
const char* SectionTracker::seedonlypropstr = "Seed only propagation";

SectionTracker::SectionTracker( EM::EMObject& emobj,
				const EM::SectionID& sectionid,
				SectionSourceSelector* sel,
				SectionExtender* ext,
				SectionAdjuster* adj )
    : emobject_( emobj )
    , sid_( sectionid )
    , selector_(sel)
    , extender_(ext)
    , adjuster_(adj)
    , useadjuster_(true)
    , seedonlypropagation_(false)
    , displayas_(*new Attrib::SelSpec)
{
    emobject_.ref();
    init();
}


SectionTracker::~SectionTracker()
{
    emobject_.unRef();
    delete selector_;
    delete extender_;
    delete adjuster_;
    delete &displayas_;
}


EM::SectionID SectionTracker::sectionID() const
{ return sid_; }


bool SectionTracker::init() { return true; }


void SectionTracker::reset()
{
    if ( selector_ ) selector_->reset();
    if ( extender_ ) extender_->reset();
    if ( adjuster_ ) adjuster_->reset();
}


void SectionTracker::removeUnSupported( TypeSet<EM::SubID>& subids ) const
{
    if ( !emobject_.isGeometryChecksEnabled() )
	return;

    mDynamicCastGet(const Geometry::ParametricSurface*, gesurf,
	const_cast<const EM::EMObject&>(emobject_).sectionGeometry(sid_) );
    bool change = true;
    while ( gesurf && change )
    {
	change = false;
	for ( int idx=0; idx<subids.size(); idx++ )
	{
	    if ( !gesurf->hasSupport(RowCol::fromInt64(subids[idx])) )
	    {
		const EM::PosID pid( emobject_.id(), sid_, subids[idx] );
		emobject_.unSetPos(pid,false);
		subids.removeSingle(idx);
		idx--;
		change = true;
	    }
	}
    }
}


bool SectionTracker::erasePositions( const TypeSet<EM::SubID>& origsubids,
				     const TypeSet<EM::SubID>& excludedpos,
				     bool addtoundo ) const
{
    TypeSet<EM::SubID> subids( origsubids );
    EM::PosID pid(emobject_.id(),sid_,0 );

    bool change = true;
    while ( change )
    {
	change = false;
	for ( int idx=0; idx<subids.size(); idx++ )
	{
	    pid.setSubID(subids[idx]);
	    if ( excludedpos.isPresent(subids[idx]) ||
		 emobject_.unSetPos(pid,addtoundo) )
	    {
		subids.removeSingle(idx--);
		change = true;
	    }
	}
    }

    return subids.size() ? false : true;
}


void SectionTracker::getLockedSeeds( TypeSet<EM::SubID>& lockedseeds )
{
    lockedseeds.erase();
    if ( !emobject_.isPosAttribLocked( EM::EMObject::sSeedNode() ) )
	return;

    const TypeSet<EM::PosID>* seedlist =
	emobject_.getPosAttribList( EM::EMObject::sSeedNode() );
    const int nrseeds = seedlist ? seedlist->size() : 0;

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const Coord3 seedpos = emobject_.getPos( (*seedlist)[idx] );
	const BinID seedbid = SI().transform( seedpos );
	if ( (*seedlist)[idx].sectionID()==sid_ &&
	     engine().activeVolume().hsamp_.includes(seedbid) )
	{
	    lockedseeds += (*seedlist)[idx].subID();
	}
    }
}


#define mAction(function, actionobj ) \
bool SectionTracker::function() \
{ \
    if ( !actionobj ) return true; \
 \
    while ( int res = actionobj->nextStep() ) \
    { \
	if ( res==-1 ) \
	{ \
	    errmsg = actionobj->errMsg(); \
	    return false; \
	} \
    }  \
 \
    return true; \
}

bool SectionTracker::select()
{
    if ( !selector_ ) return true;

    while ( int res = selector_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = selector_->errMsg();
	    return false;
	}
    }

    return true;
}


bool SectionTracker::extend()
{
    if ( !extender_ ) return true;

    if ( selector_ )
	extender_->setStartPositions( selector_->selectedPositions() );

    while ( int res = extender_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = extender_->errMsg();
	    return false;
	}
    }

    return true;
}


bool SectionTracker::adjust()
{
    if ( !adjuster_ ) return true;

    if ( extender_ )
	adjuster_->setPositions( extender_->getAddedPositions(),
    				 &extender_->getAddedPositionsSource() );

    emobject_.setBurstAlert( true );
    while ( int res = adjuster_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = adjuster_->errMsg();
	    emobject_.setBurstAlert( false );
	    return false;
	}
    }

    emobject_.setBurstAlert( false );
    return true;
}


#define mGet( clss, func, name ) \
clss* SectionTracker::func() { return name; }  \
const clss* SectionTracker::func() const \
{ return const_cast<SectionTracker*>(this)->func(); }


mGet( SectionSourceSelector, selector, selector_ )
mGet( SectionExtender, extender, extender_ )
mGet( SectionAdjuster, adjuster, adjuster_ )

const char* SectionTracker::errMsg() const
{ return errmsg_.str(); }


TrcKeyZSampling
	SectionTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    return adjuster_ ? adjuster_->getAttribCube(spec) : engine().activeVolume();
}


void SectionTracker::getNeededAttribs( TypeSet<Attrib::SelSpec>& res ) const
{
    if ( adjuster_ ) adjuster_->getNeededAttribs( res );
}


void SectionTracker::useAdjuster(bool yn) { useadjuster_=yn; }


bool SectionTracker::adjusterUsed()  const { return useadjuster_; }


void SectionTracker::setSetupID( const MultiID& id ) { setupid_=id; }


const MultiID& SectionTracker::setupID() const { return setupid_; }


bool SectionTracker::hasInitializedSetup() const
{
    return ( !adjuster_ || adjuster_->hasInitializedSetup() );
}


void SectionTracker::setDisplaySpec( const Attrib::SelSpec& as )
{ displayas_ = as; }


const Attrib::SelSpec& SectionTracker::getDisplaySpec() const
{ return displayas_; }


void SectionTracker::setSeedOnlyPropagation( bool yn )
{ seedonlypropagation_ = yn; }


bool SectionTracker::propagatingFromSeedOnly() const
{ return seedonlypropagation_; }


void SectionTracker::fillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( useadjusterstr, adjusterUsed() );
    trackpar.setYN( seedonlypropstr, seedonlypropagation_ );

    par.mergeComp( trackpar, trackerstr );
    if ( selector_ ) selector_->fillPar( par );
    if ( extender_ ) extender_->fillPar( par );
    if ( adjuster_ ) adjuster_->fillPar( par );
}


bool SectionTracker::usePar( const IOPar& par )
{
    PtrMan<IOPar> trackpar = par.subselect( trackerstr );
    bool dummy = true;
    if ( trackpar ) trackpar->getYN( useadjusterstr, dummy );
    useAdjuster( dummy );
    if ( trackpar ) trackpar->getYN( seedonlypropstr, seedonlypropagation_ );

    bool res = true;
    if ( selector_ && !selector_->usePar(par) )
	res = false;
    if ( extender_ && !extender_->usePar(par) )
	res = false;
    if ( adjuster_ && !adjuster_->usePar(par) )
	res = false;

    return res;
}

} // namespace MPE
