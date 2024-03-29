/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sectionadjuster.h"

#include "iopar.h"
#include "mpeengine.h"
#include "ptrman.h"

namespace MPE
{

const char* SectionAdjuster::sKeyAdjuster() { return "Adjuster"; }
const char* SectionAdjuster::sKeyThreshold() { return "Threshold value"; }
const char* SectionAdjuster::sKeyRemoveOnFailure(){ return "Remove on Failure";}


SectionAdjuster::SectionAdjuster()
{
}


SectionAdjuster::~SectionAdjuster()
{
}


EM::SectionID SectionAdjuster::sectionID() const
{ return EM::SectionID::def(); }


void SectionAdjuster::setPositions( const TypeSet<TrcKey>& p,
				    const TypeSet<TrcKey>* src )
{
    tks_ = p;
    if ( src )
	tksrc_ = *src;
    else
	tksrc_.erase();
}


void SectionAdjuster::setSeedPosition( const TrcKey& tk )
{
    seedtk_ = tk;
}


int SectionAdjuster::nextStep()
{ return 0; }


const char* SectionAdjuster::errMsg() const
{ return errmsg_.str(); }


TrcKeyZSampling
	SectionAdjuster::getAttribCube( const Attrib::SelSpec& ) const
{ return engine().activeVolume(); }


void SectionAdjuster::getNeededAttribs( TypeSet<Attrib::SelSpec>& ) const
{}


void SectionAdjuster::setThresholdValue( float val )
{ thresholdval_ = val; }


float SectionAdjuster::getThresholdValue() const
{ return thresholdval_; }


bool SectionAdjuster::removeOnFailure(bool yn)
{
    const bool res = removeonfailure_;
    removeonfailure_ = yn;
    return res;
}


bool SectionAdjuster::removesOnFailure() const
{ return removeonfailure_; }


void SectionAdjuster::fillPar( IOPar& par ) const
{
    IOPar adjpar;
    adjpar.set( sKeyThreshold(), thresholdval_ );
    adjpar.setYN( sKeyRemoveOnFailure(), removeonfailure_ );
    par.mergeComp( adjpar, sKeyAdjuster() );
}


bool SectionAdjuster::usePar( const IOPar& par )
{
    PtrMan<IOPar> adjpar = par.subselect( sKeyAdjuster() );
    if ( adjpar )
    {
	adjpar->get( sKeyThreshold(), thresholdval_ );
	adjpar->getYN( sKeyRemoveOnFailure(), removeonfailure_ );
    }

    return true;
}


void SectionAdjuster::setSeedId( int seedid )
{
    seedid_ = seedid;
}

} // namespace MPE
