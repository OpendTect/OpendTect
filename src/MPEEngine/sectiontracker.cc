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
				SectionSourceSelector* selector__,
				SectionExtender* extender__,
				SectionAdjuster* adjuster__ )
    : emobject( emobj )
    , sid( sectionid )
    , selector_(selector__)
    , extender_(extender__)
    , adjuster_(adjuster__)
    , useadjuster(true)
    , seedonlypropagation(false)
    , displayas(*new Attrib::SelSpec)
{
    emobject.ref();
    init();
}


SectionTracker::~SectionTracker()
{
    emobject.unRef();
    delete selector_;
    delete extender_;
    delete adjuster_;
    delete &displayas;
}


EM::SectionID SectionTracker::sectionID() const
{ return sid; }


bool SectionTracker::init() { return true; }


void SectionTracker::reset()
{
    if ( selector_ ) selector_->reset();
    if ( extender_ ) extender_->reset();
    if ( adjuster_ ) adjuster_->reset();
}


bool SectionTracker::trackWithPlane( const TrackPlane& plane )
{
    if ( !selector_ && !extender_ )
    {
	errmsg = "Internal: No selector or extender available.";
	return false;
    }

    reset();

    selector_->setTrackPlane( plane );
    if ( !select() )
	return false;

    TypeSet<EM::SubID> lockedseeds;
    getLockedSeeds( lockedseeds );

    if ( plane.getTrackMode()==TrackPlane::Erase )
    {
	if ( !erasePositions(selector_->selectedPositions(),lockedseeds,true) )
	{
	    errmsg = "Could not remove all nodes since that would "
		     "divide the section in multiple parts.";
	    return false;
	}

	return true;
    }

    extender_->setDirection( plane.motion() );
    extender_->excludePositions( &lockedseeds );
    const bool res = extend();
    extender_->excludePositions(0);
    if ( !res ) 
	return false;

    TypeSet<EM::SubID> addedpos = extender_->getAddedPositions();

    if ( adjusterUsed() && !adjust() )
	return false;

    removeUnSupported( addedpos );
    return true;
}


void SectionTracker::removeUnSupported( TypeSet<EM::SubID>& subids ) const
{
    if ( !emobject.isGeometryChecksEnabled() )
	return;

    mDynamicCastGet(const Geometry::ParametricSurface*, gesurf,
	const_cast<const EM::EMObject&>(emobject).sectionGeometry(sid) );
    bool change = true;
    while ( gesurf && change )
    {
	change = false;
	for ( int idx=0; idx<subids.size(); idx++ )
	{
	    if ( !gesurf->hasSupport(RowCol::fromInt64(subids[idx])) )
	    {
		const EM::PosID pid( emobject.id(), sid, subids[idx] );
		emobject.unSetPos(pid,false);
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
    EM::PosID pid(emobject.id(),sid,0 );

    bool change = true;
    while ( change )
    {
	change = false;
	for ( int idx=0; idx<subids.size(); idx++ )
	{
	    pid.setSubID(subids[idx]);
	    if ( excludedpos.indexOf(subids[idx])!=-1 || 
		 emobject.unSetPos(pid,addtoundo) )
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
    if ( !emobject.isPosAttribLocked( EM::EMObject::sSeedNode() ) )
	return;

    const TypeSet<EM::PosID>* seedlist = 
	emobject.getPosAttribList( EM::EMObject::sSeedNode() );
    const int nrseeds = seedlist ? seedlist->size() : 0;

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const Coord3 seedpos = emobject.getPos( (*seedlist)[idx] );
	const BinID seedbid = SI().transform( seedpos );
	if ( (*seedlist)[idx].sectionID()==sid && 
	     engine().activeVolume().hrg.includes(seedbid) )
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
	    errmsg = selector_->errMsg();
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
	    errmsg = extender_->errMsg();
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

    emobject.setBurstAlert( true );
    while ( int res = adjuster_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg = adjuster_->errMsg();
	    emobject.setBurstAlert( false );
	    return false;
	}
    }

    emobject.setBurstAlert( false );
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
{ return errmsg.str(); }


CubeSampling SectionTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    return adjuster_ ? adjuster_->getAttribCube(spec) : engine().activeVolume();
}


void SectionTracker::getNeededAttribs( 
				ObjectSet<const Attrib::SelSpec>& res ) const
{
    if ( adjuster_ ) adjuster_->getNeededAttribs( res );
}


void SectionTracker::useAdjuster(bool yn) { useadjuster=yn; }


bool SectionTracker::adjusterUsed()  const { return useadjuster; }


void SectionTracker::setSetupID( const MultiID& id ) { setupid=id; }


const MultiID& SectionTracker::setupID() const { return setupid; }


bool SectionTracker::hasInitializedSetup() const
{
    return ( !adjuster_ || adjuster_->hasInitializedSetup() );
}


void SectionTracker::setDisplaySpec( const Attrib::SelSpec& as )
{ displayas = as; }


const Attrib::SelSpec& SectionTracker::getDisplaySpec() const
{ return displayas; }


void SectionTracker::setSeedOnlyPropagation( bool yn )
{ seedonlypropagation = yn; }


bool SectionTracker::propagatingFromSeedOnly() const
{ return seedonlypropagation; }


void SectionTracker::fillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( useadjusterstr, adjusterUsed() );
    trackpar.setYN( seedonlypropstr, seedonlypropagation );

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
    if ( trackpar ) trackpar->getYN( seedonlypropstr, seedonlypropagation );

    bool res = true;
    if ( selector_ && !selector_->usePar(par) )
	res = false;
    if ( extender_ && !extender_->usePar(par) )
	res = false;
    if ( adjuster_ && !adjuster_->usePar(par) )
	res = false;

    return res;
}

}; // namespace MPE
