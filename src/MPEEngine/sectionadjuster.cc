/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: sectionadjuster.cc,v 1.9 2005-10-18 17:10:17 cvskris Exp $
________________________________________________________________________

-*/


#include "sectionadjuster.h"

#include "mpeengine.h"
#include "iopar.h"
#include "positionscorecomputer.h"

namespace MPE
{

const char* SectionAdjuster::sKeyAdjuster() { return "Adjuster"; }
const char* SectionAdjuster::sKeyThreshold() { return "Threshold value"; }
const char* SectionAdjuster::sKeyRemoveOnFailure(){ return "Remove on Failure";}

SectionAdjuster::SectionAdjuster( const EM::SectionID& sid )
    : sectionid_(sid)
    , removeonfailure_(true)
    , thresholdval_(0.5)
    , refpos_(0)
{}


EM::SectionID SectionAdjuster::sectionID() const { return sectionid_; }


void SectionAdjuster::setPositions(const TypeSet<EM::SubID>& p,
       				   const TypeSet<EM::SubID>* src )
{
    refpos_ = 0;
    pids_ = p;
    if ( src ) pidsrc_ = *src;
    else pidsrc_.erase();
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


bool SectionAdjuster::removeOnFailure(bool yn)
{
    const bool res = removeonfailure_;
    removeonfailure_ = yn;
    return res;
}


bool SectionAdjuster::removesOnFailure() const { return removeonfailure_; }


void SectionAdjuster::fillPar( IOPar& par ) const
{
    IOPar adjpar;
    adjpar.set( sKeyThreshold(), thresholdval_ );
    adjpar.setYN( sKeyRemoveOnFailure(), removeonfailure_ );
    par.mergeComp( adjpar, sKeyAdjuster() );

    for ( int idx=0; idx<nrComputers(); idx++ )
	getComputer(idx)->fillPar( par );
}


bool SectionAdjuster::usePar( const IOPar& par )
{
    PtrMan<IOPar> adjpar = par.subselect( sKeyAdjuster() );
    if ( adjpar )
    {
	adjpar->get( sKeyThreshold(), thresholdval_ );
	adjpar->getYN( sKeyRemoveOnFailure(), removeonfailure_ );
    }
    
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	if ( !getComputer(idx)->usePar(par) )
	    return false;
    }

    return true;
}

}; // namespace MPE
