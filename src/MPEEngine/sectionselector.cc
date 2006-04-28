/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionselector.cc,v 1.2 2006-04-28 20:59:59 cvskris Exp $";

#include "sectionselectorimpl.h"

namespace MPE 
{
SectionSourceSelector::SectionSourceSelector( const EM::SectionID& sid )
    : sectionid_( sid )
{}


EM::SectionID SectionSourceSelector::sectionID() const { return sectionid_; }


void SectionSourceSelector::reset() { selpos_.erase(); }


void SectionSourceSelector::setTrackPlane( const MPE::TrackPlane& ) {}


int SectionSourceSelector::nextStep() { return 0; }


const char* SectionSourceSelector::errMsg() const
{ return errmsg_[0] ? errmsg_ : 0; }


const TypeSet<EM::SubID>& SectionSourceSelector::selectedPositions() const
{ return selpos_;}



};
