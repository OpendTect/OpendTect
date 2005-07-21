/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.4 2005-07-21 20:57:38 cvskris Exp $
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

SectionAdjuster::SectionAdjuster( const EM::SectionID& sid )
    : sectionid_(sid)
    , stopbelowthrhold_(false)
    , thresholdval_(0.5)
    , direction( 0 )
{}


EM::SectionID SectionAdjuster::sectionID() const { return sectionid_; }


void SectionAdjuster::setPositions(const TypeSet<EM::SubID>& p ) { pids = p; }


void SectionAdjuster::setDirection(const BinIDValue* d) { direction = d; }


void SectionAdjuster::reset() { direction = 0; }


int SectionAdjuster::nextStep() { return 0; }


const char* SectionAdjuster::errMsg() const { return errmsg_[0] ? errmsg_ : 0; }


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


void SectionAdjuster::setThresholdValue(float val) { thresholdval_ = val; }


float SectionAdjuster::getThresholdValue() const { return thresholdval_; }


void SectionAdjuster::doStopBelowThreshold(bool yn) { stopbelowthrhold_ = yn; }


bool SectionAdjuster::stopBelowThreshold() const { return stopbelowthrhold_; }


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
