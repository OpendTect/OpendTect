/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sectionselectorimpl.h"

namespace MPE
{

SectionSourceSelector::SectionSourceSelector()
{}


SectionSourceSelector::~SectionSourceSelector()
{}


void SectionSourceSelector::reset()
{
    selpos_.erase();
}


int SectionSourceSelector::nextStep()
{
    return 0;
}


const char* SectionSourceSelector::errMsg() const
{
    return errmsg_.str();
}


const TypeSet<TrcKey>& SectionSourceSelector::selectedPositions() const
{
    return selpos_;
}

} // namespace MPE
