/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.2 2005-03-14 16:43:13 cvsnanne Exp $
________________________________________________________________________

-*/


#include "sectionadjuster.h"
#include "positionscorecomputer.h"
#include "iopar.h"

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


void SectionAdjuster::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrComputers(); idx++ )
	getComputer(idx)->fillPar( par );
}


bool SectionAdjuster::usePar( const IOPar& par )
{
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	if ( !getComputer(idx)->usePar(par) ) return false;
    }

    return true;
}

}; // namespace MPE
