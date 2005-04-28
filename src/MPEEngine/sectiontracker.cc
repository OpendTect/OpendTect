/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectiontracker.cc,v 1.7 2005-04-28 15:39:55 cvsnanne Exp $";

#include "sectiontracker.h"

#include "sectionextender.h"
#include "sectionselector.h"
#include "sectionadjuster.h"
#include "positionscorecomputer.h"
#include "iopar.h"
#include "ptrman.h"

namespace MPE 
{

const char* SectionTracker::trackerstr = "Tracker";
const char* SectionTracker::useadjusterstr = "Use adjuster";

SectionTracker::SectionTracker( SectionSourceSelector* selector__,
				SectionExtender* extender__,
				SectionAdjuster* adjuster__ )
    : selector_(selector__)
    , extender_(extender__)
    , adjuster_(adjuster__)
    , useadjuster_(true)
    , displayas_(*new AttribSelSpec)
{
    init();
}


SectionTracker::~SectionTracker()
{
    delete selector_;
    delete extender_;
    delete adjuster_;
    delete &displayas_;
}


EM::SectionID SectionTracker::sectionID() const
{
    if ( selector_&&selector_->sectionID()!=-1 ) return selector_->sectionID();
    if ( extender_&&extender_->sectionID()!=-1 ) return extender_->sectionID();
    if ( adjuster_&&adjuster_->sectionID()!=-1 ) return adjuster_->sectionID();

    return -1;
}


bool SectionTracker::init()
{
    if ( extender_ ) extender_->setSelector( selector_ );
    if ( adjuster_ ) adjuster_->setExtender( extender_ );
    return true;
}


void SectionTracker::reset()
{
    if ( selector_ ) selector_->reset();
    if ( extender_ ) extender_->reset();
    if ( adjuster_ ) adjuster_->reset();
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
	    errmsg_ = actionobj->errMsg(); \
	    return false; \
	} \
    }  \
 \
    return true; \
}

mAction( select, selector_ )
mAction( extend, extender_ )
mAction( adjust, adjuster_ )

#define mGet( clss, func, name ) \
clss* SectionTracker::func() { return name; }  \
const clss* SectionTracker::func() const \
{ return const_cast<SectionTracker*>(this)->func(); }


mGet( SectionSourceSelector, selector, selector_ )
mGet( SectionExtender, extender, extender_ )
mGet( SectionAdjuster, adjuster, adjuster_ )

const char* SectionTracker::errMsg() const
{ return errmsg_[0] ? (const char*) errmsg_ : 0; }


void SectionTracker::getNeededAttribs( 
				ObjectSet<const AttribSelSpec>& res ) const
{
    if ( !adjuster_ ) return;
    for ( int idx=0; idx<adjuster_->nrComputers(); idx++ )
    {
	PositionScoreComputer* psc = adjuster_->getComputer( idx );
	for ( int asidx=0; asidx<psc->nrAttribs(); asidx++ )
	{
	    const AttribSelSpec* as = psc->getSelSpec( asidx );
	    if ( indexOf(res,*as) < 0 )
		res += as;
	}
    }
}


void SectionTracker::setDisplaySpec( const AttribSelSpec& as )
{ displayas_ = as; }

const AttribSelSpec& SectionTracker::getDisplaySpec() const
{ return displayas_; }


void SectionTracker::fillPar( IOPar& par ) const
{
    IOPar trackpar;
    trackpar.setYN( useadjusterstr, useadjuster_ );
    par.mergeComp( trackpar, trackerstr );
    if ( adjuster_ ) adjuster_->fillPar( par );
}


bool SectionTracker::usePar( const IOPar& par )
{
    PtrMan<IOPar> trackpar = par.subselect( trackerstr );
    useadjuster_ = true;
    if ( trackpar )
	trackpar->getYN( useadjusterstr, useadjuster_ );
	
    if ( adjuster_ ) adjuster_->usePar( par );

    return true;
}

}; // namespace MPE
