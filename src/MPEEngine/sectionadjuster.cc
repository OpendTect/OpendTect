/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.6 2005-08-01 07:09:28 cvsnanne Exp $
________________________________________________________________________

-*/


#include "sectionadjuster.h"

#include "mpeengine.h"
#include "iopar.h"
#include "positionscorecomputer.h"

namespace MPE
{

const char* SectionAdjuster::adjusterstr = "Adjuster";
const char* SectionAdjuster::thresholdstr = "Threshold value";
const char* SectionAdjuster::stoptrackstr = "Stop tracking below threshold";

SectionAdjuster::SectionAdjuster( const EM::SectionID& sid )
    : sectionid_(sid)
    , stopbelowthrhold_(false)
    , thresholdval_(0.5)
{}


EM::SectionID SectionAdjuster::sectionID() const { return sectionid_; }


void SectionAdjuster::setPositions(const TypeSet<EM::SubID>& p,
       				   const TypeSet<EM::SubID>* src )
{
    pids = p;
    if ( src ) pidsrc = *src;
    else pidsrc.erase();
}


int SectionAdjuster::nextStep() { return 0; }


const char* SectionAdjuster::errMsg() const { return errmsg_[0] ? errmsg_ : 0; }


CubeSampling SectionAdjuster::getAttribCube( const Attrib::SelSpec& spec ) const
{
    const CubeSampling activearea( engine().activeVolume() );
    CubeSampling res( activearea );
    for ( int idx=0; idx<computers_.size(); idx++ )
	res.include( computers_[idx]->getAttribCube(activearea) );

    return res;
}


void SectionAdjuster::getNeededAttribs(
	ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int idx=0; idx<computers_.size(); idx++ )
    {
	PositionScoreComputer* psc = computers_[idx];
	for ( int asidx=0; asidx<psc->nrAttribs(); asidx++ )
	{
	    const Attrib::SelSpec* as = psc->getSelSpec( asidx );
	    if ( as && indexOf(res,*as) < 0 )
		res += as;
	}
    }
}


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
