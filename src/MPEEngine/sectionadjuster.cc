/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.3 2005-03-17 14:55:46 cvsnanne Exp $
________________________________________________________________________

-*/


#include "sectionadjuster.h"
#include "positionscorecomputer.h"
#include "iopar.h"

namespace MPE
{

const char* SectionAdjuster::adjusterstr = "Adjuster";
const char* SectionAdjuster::thresholdstr = "Threshold value";
const char* SectionAdjuster::stoptrackstr = "Stop tracking below threshold";


PositionScoreComputer* SectionAdjuster::getComputer( int idx )
{
    return computers_[idx];
}


const PositionScoreComputer* SectionAdjuster::getComputer( int idx ) const
{
    return const_cast<SectionAdjuster*>(this)->getComputer(idx);
}


int SectionAdjuster::nrComputers() const
{
    return computers_.size();
}


void SectionAdjuster::fillPar( IOPar& par ) const
{
    IOPar adjpar;
    adjpar.set( thresholdstr, thresholdval_ );
    adjpar.setYN( stoptrackstr, stopbelowthrhold_ );
    par.mergeComp( adjpar, adjusterstr );

    for ( int idx=0; idx<nrComputers(); idx++ )
	getComputer(idx)->fillPar( par );
}


bool SectionAdjuster::usePar( const IOPar& par )
{
    stopbelowthrhold_ = false;
    thresholdval_ = 0.5;
    PtrMan<IOPar> adjpar = par.subselect( adjusterstr );
    if ( adjpar )
    {
	adjpar->get( thresholdstr, thresholdval_ );
	adjpar->getYN( stoptrackstr, stopbelowthrhold_ );
    }
    
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	if ( !getComputer(idx)->usePar(par) )
	    return false;
    }

    return true;
}

}; // namespace MPE
