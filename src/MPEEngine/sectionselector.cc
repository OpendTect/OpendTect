/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionselector.cc,v 1.1 2005-07-18 16:58:46 cvskris Exp $";

#include "sectionselectorimpl.h"

namespace MPE 
{
SectionSourceSelector::SectionSourceSelector( const EM::SectionID& sid )
    : sectionid( sid )
{}


EM::SectionID SectionSourceSelector::sectionID() const { return sectionid; }


void SectionSourceSelector::reset() { selpos.erase(); }


void SectionSourceSelector::setTrackPlane( const MPE::TrackPlane& ) {}


int SectionSourceSelector::nextStep() { return 0; }


const char* SectionSourceSelector::errMsg() const
{ return errmsg[0] ? errmsg : 0; }


const TypeSet<EM::SubID>& SectionSourceSelector::selectedPositions() const
{ return selpos;}



};
