/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

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
{ return errmsg_.str(); }


const TypeSet<EM::SubID>& SectionSourceSelector::selectedPositions() const
{ return selpos_;}



};
