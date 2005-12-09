/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectiontracker.cc,v 1.14 2005-12-09 16:52:37 cvskris Exp $";

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

namespace MPE 
{

const char* SectionTracker::trackerstr = "Tracker";
const char* SectionTracker::useadjusterstr = "Use adjuster";

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

    if ( plane.getTrackMode()==TrackPlane::Erase )
    {
	if ( !erasePositions(selector_->selectedPositions(),true) )
	{
	    errmsg = "Could not remove all nodes since that would "
		     "divide the section in multiple parts.";
	    return false;
	}

	return true;
    }

    extender_->setDirection(plane.motion());
    if ( !extend() )
	return false;

    TypeSet<EM::SubID> addedpos = extender_->getAddedPositions();

    if ( adjusterUsed() && !adjust() )
	return false;

    removeUnSupported( addedpos );
    return true;
}


void SectionTracker::removeUnSupported( TypeSet<EM::SubID>& subids ) const
{
    mDynamicCastGet(const Geometry::ParametricSurface*, gesurf,
	const_cast<const EM::EMObject&>(emobject).getElement(sid) );
    bool change = true;
    while ( gesurf && change )
    {
	change = false;
	for ( int idx=0; idx<subids.size(); idx++ )
	{
	    if ( !gesurf->hasSupport(RowCol(subids[idx])) )
	    {
		const EM::PosID pid( emobject.id(), sid, subids[idx] );
		emobject.unSetPos(pid,false);
		subids.remove(idx);
		idx--;
		change = true;
	    }
	}
    }
}



bool SectionTracker::erasePositions( const TypeSet<EM::SubID>& origsubids,
				     bool addtohistory ) const
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
	    if ( emobject.unSetPos(pid,addtohistory) )
	    {
		subids.remove(idx--);
		change = true;
	    }
	}
    }

    return subids.size() ? false : true;
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

    while ( int res = adjuster_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg = adjuster_->errMsg();
	    return false;
	}
    }

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
{ return errmsg[0] ? (const char*) errmsg : 0; }


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


void SectionTracker::setDisplaySpec( const Attrib::SelSpec& as )
{ displayas = as; }


const Attrib::SelSpec& SectionTracker::getDisplaySpec() const
{ return displayas; }


void SectionTracker::fillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( useadjusterstr, adjusterUsed() );
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
	
    if ( selector_ && !selector_->usePar(par) ) return false;
    if ( extender_ && !extender_->usePar(par) ) return false;
    if ( adjuster_ && !adjuster_->usePar(par) ) return false;

    return true;
}

}; // namespace MPE
