/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectiontracker.cc,v 1.1 2005-01-06 09:25:21 kristofer Exp $";

#include "sectiontracker.h"

#include "sectionextender.h"
#include "sectionselector.h"
#include "sectionadjuster.h"

namespace MPE 
{


SectionTracker::SectionTracker( SectionSourceSelector* selector__,
				SectionExtender* extender__,
				SectionAdjuster* adjuster__ )
    : selector_( selector__ )
    , extender_( extender__ )
    , adjuster_( adjuster__ )
{
    init();
}


SectionTracker::~SectionTracker()
{
    delete selector_;
    delete extender_;
    delete adjuster_;
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
    if ( !actionobj ) return false; \
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
{ return errmsg[0] ? (const char*) errmsg : 0; }

}; //Namespace
