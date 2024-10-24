/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sectiontracker.h"

#include "attribsel.h"
#include "emobject.h"
#include "iopar.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "survinfo.h"

const char* MPE::SectionTracker::trackerstr = "Tracker";
const char* MPE::SectionTracker::useadjusterstr = "Use adjuster";
const char* MPE::SectionTracker::seedonlypropstr = "Seed only propagation";

MPE::SectionTracker::SectionTracker( EM::EMObject& emobj,
				     SectionSourceSelector* sel,
				     SectionExtender* ext,
				     SectionAdjuster* adj )
    : emobject_( &emobj )
    , displayas_(*new Attrib::SelSpec)
    , selector_(sel)
    , extender_(ext)
    , adjuster_(adj)
{
    init();
}


MPE::SectionTracker::~SectionTracker()
{
    delete selector_;
    delete extender_;
    delete adjuster_;
    delete &displayas_;
}


ConstRefMan<EM::EMObject> MPE::SectionTracker::emObject() const
{
    return emobject_.get();
}


RefMan<EM::EMObject> MPE::SectionTracker::emObject()
{
    return emobject_.get();
}


bool MPE::SectionTracker::init()
{ return true; }


void MPE::SectionTracker::reset()
{
    if ( selector_ ) selector_->reset();
    if ( extender_ ) extender_->reset();
    if ( adjuster_ ) adjuster_->reset();
}


void MPE::SectionTracker::getLockedSeeds( TypeSet<EM::SubID>& lockedseeds )
{
    RefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    lockedseeds.erase();
    if ( !emobject->isPosAttribLocked( EM::EMObject::sSeedNode() ) )
	return;

    const TypeSet<EM::PosID>* seedlist =
			emobject->getPosAttribList( EM::EMObject::sSeedNode() );
    const int nrseeds = seedlist ? seedlist->size() : 0;

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const Coord3 seedpos = emobject->getPos( (*seedlist)[idx] );
	const BinID seedbid = SI().transform( seedpos );
	if ( engine().activeVolume().hsamp_.includes(seedbid) )
	{
	    lockedseeds += (*seedlist)[idx].subID();
	}
    }
}


bool MPE::SectionTracker::select()
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


bool MPE::SectionTracker::extend()
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


bool MPE::SectionTracker::adjust()
{
    if ( !adjuster_ )
	return true;

    if ( extender_ )
	adjuster_->setPositions( extender_->getAddedPositions(),
				 &extender_->getAddedPositionsSource() );

    RefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return false;

    emobject->setBurstAlert( true );
    while ( int res = adjuster_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = adjuster_->errMsg();
	    emobject->setBurstAlert( false );
	    return false;
	}
    }

    emobject->setBurstAlert( false );
    return true;
}


#define mGet( clss, func, name ) \
clss* MPE::SectionTracker::func() { return name; }  \
const clss* MPE::SectionTracker::func() const \
{ return mSelf().func(); }


mGet( MPE::SectionSourceSelector, selector, selector_ )
mGet( MPE::SectionExtender, extender, extender_ )
mGet( MPE::SectionAdjuster, adjuster, adjuster_ )

const char* MPE::SectionTracker::errMsg() const
{ return errmsg_.str(); }


TrcKeyZSampling
	MPE::SectionTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    return adjuster_ ? adjuster_->getAttribCube(spec) : engine().activeVolume();
}


void MPE::SectionTracker::getNeededAttribs( TypeSet<Attrib::SelSpec>& res) const
{
    if ( adjuster_ )
	adjuster_->getNeededAttribs( res );
}


void MPE::SectionTracker::useAdjuster(bool yn)
{ useadjuster_=yn; }

bool MPE::SectionTracker::adjusterUsed() const
{ return useadjuster_; }

void MPE::SectionTracker::setSetupID( const MultiID& id )
{ setupid_ = id; }

const MultiID& MPE::SectionTracker::setupID() const
{ return setupid_; }

bool MPE::SectionTracker::hasInitializedSetup() const
{ return ( !adjuster_ || adjuster_->hasInitializedSetup() ); }

void MPE::SectionTracker::setDisplaySpec( const Attrib::SelSpec& as )
{ displayas_ = as; }

const Attrib::SelSpec& MPE::SectionTracker::getDisplaySpec() const
{ return displayas_; }

void MPE::SectionTracker::setSeedOnlyPropagation( bool yn )
{ seedonlypropagation_ = yn; }

bool MPE::SectionTracker::propagatingFromSeedOnly() const
{ return seedonlypropagation_; }


void MPE::SectionTracker::fillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( useadjusterstr, adjusterUsed() );
    trackpar.setYN( seedonlypropstr, seedonlypropagation_ );

    par.mergeComp( trackpar, trackerstr );
    if ( selector_ ) selector_->fillPar( par );
    if ( extender_ ) extender_->fillPar( par );
    if ( adjuster_ ) adjuster_->fillPar( par );
}


bool MPE::SectionTracker::usePar( const IOPar& par )
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


// Deprecated impls:

MPE::SectionTracker::SectionTracker( EM::EMObject& emobj, const EM::SectionID&,
				SectionSourceSelector* sss,
				SectionExtender* se,
				SectionAdjuster* sa )
    : SectionTracker(emobj,sss,se,sa)
{}


EM::SectionID MPE::SectionTracker::sectionID() const
{ return EM::SectionID::def(); }
