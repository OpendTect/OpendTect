/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.1 2005-03-11 16:55:15 cvsnanne Exp $
________________________________________________________________________

-*/


#include "sectionadjuster.h"
#include "positionscorecomputer.h"

namespace MPE
{

PositionScoreComputer* SectionAdjuster::getComputer( int idx )
{
    return computers[idx];
}


const PositionScoreComputer* SectionAdjuster::getComputer( int idx ) const
{
    return const_cast<SectionAdjuster*>(this)->getComputer(idx);
}


int SectionAdjuster::nrComputers() const
{
    return computers.size();
}

}; // namespace MPE
