/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "sectionselectorimpl.h"

namespace MPE
{

SectionSourceSelector::SectionSourceSelector( const EM::SectionID& sid )
    : sectionid_( sid )
{}


EM::SectionID SectionSourceSelector::sectionID() const { return sectionid_; }

void SectionSourceSelector::reset() { selpos_.erase(); }

int SectionSourceSelector::nextStep() { return 0; }

const char* SectionSourceSelector::errMsg() const
{ return errmsg_.str(); }

const TypeSet<TrcKey>& SectionSourceSelector::selectedPositions() const
{ return selpos_;}

} // namespace MPE
