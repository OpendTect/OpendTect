/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionselector.cc,v 1.3 2009-07-22 16:01:34 cvsbert Exp $";

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
