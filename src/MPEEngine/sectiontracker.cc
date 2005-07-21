/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectiontracker.cc,v 1.9 2005-07-21 20:57:38 cvskris Exp $";

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


bool SectionTracker::init() { return true; }


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
	adjuster_->setPositions( extender_->getAddedPositions() );

    while ( int res = adjuster_->nextStep() )
    {
	if ( res==-1 )
	{
	    errmsg_ = adjuster_->errMsg();
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
	    if ( as && indexOf(res,*as) < 0 )
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
