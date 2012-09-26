/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "eventfreqattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "seistrc.h"
#include "seistrcprop.h"


namespace Attrib
{
    
mAttrDefCreateInstance(EventFreq)
    
void EventFreq::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::Frequency );
    desc->addOutputDataType( Seis::Phase );
    mAttrEndInitClass
}


EventFreq::EventFreq( Desc& desc_ )
    : Provider( desc_ )
    , dessamps_(-20,20)
{
    if ( !isOK() ) return;
}


const Interval<int>* EventFreq::desZSampMargin(int,int) const
{
    return &dessamps_;
}


bool EventFreq::getInputData( const BinID& relpos, int zintv )
{
    inpdata_ = inputs[0]->getData( relpos, zintv );
    if ( !inpdata_ ) return false;

    inpseries_ = inpdata_->series( getDataIndex(0) );
    cubeintv_.start = inpdata_->z0_;
    cubeintv_.stop = inpdata_->z0_ + inpdata_->nrsamples_ - 1;
    return inpseries_;
}


void EventFreq::findEvents( int z0, int nrsamples ) const
{
    Interval<int> worksamps( z0 + dessamps_.start,
	    		     z0 + dessamps_.stop + nrsamples - 1 );
    worksamps.limitTo( cubeintv_ );

    SamplingData<float> sd( cubeintv_.start, 1 );
    ValueSeriesEvFinder<float,float> evf( *inpseries_,
	    				  inpdata_->nrsamples_-1, sd );
    ValueSeriesEvent<float,float> curev( 0, worksamps.start - 2 );
    Interval<float> sampsleft( 0, worksamps.stop );

    VSEvent::Type prevtype = VSEvent::None;
    float prevpos = -999;
    while ( true )
    {
	sampsleft.start = curev.pos + 2;
	if ( sampsleft.start > sampsleft.stop )
	    break;

	curev = evf.find( VSEvent::Extr, sampsleft );
	if ( mIsUdf(curev.pos) )
	    break;
	else if ( curev.pos - prevpos < 2 )
	    // Can't be OK: below Nyquist
	    continue;
	else if ( evf.lastFound() == prevtype )
	{
	    if ( curev.pos - prevpos < 4 )
		// Too close anyway: must be noise
		continue;
	    // OK OK, seems we missed an event. Insert one in the middle
	    float prevpos = evposns_[evposns_.size()-1];
	    evposns_ += (curev.pos + prevpos) * .5;
	}

	evposns_ += curev.pos;
	prevpos = curev.pos;
	if ( prevtype == VSEvent::None )
	    firstevmax_ = evf.lastFound() == VSEvent::Max;
	prevtype = evf.lastFound();
    }
}


float EventFreq::getPDz( float* dz, int idx ) const
{
    const int sz = evposns_.size();
    const float p0 = idx > sz-1 ? evposns_[sz-1] : evposns_[idx];
    const float p1 = idx > sz-2 ? 2*p0 - evposns_[sz-2] : evposns_[idx+1];
    const float p2 = idx > sz-3 ? 2*p1 - p0 : evposns_[idx+2];
    const float p3 = idx > sz-4 ? 2*p2 - p1 : evposns_[idx+3];
    dz[0] = p1 - p0;
    dz[1] = p2 - p1;
    dz[2] = p3 - p2;
    return p1;
}


void EventFreq::fillFreqOutput( const DataHolder& output,
				int z0, int nrsamples ) const
{
    if ( evposns_.size() < 3 )
    {
	const float val = evposns_.size() < 2 ? mUdf(float)
			: 1. / (2 * refstep * (evposns_[1] - evposns_[0]));
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, val );
	return;
    }

    int evidxafter = 0;
    float dz[3];
    while ( evidxafter < evposns_.size()-1 && evposns_[evidxafter+1] < z0 )
	evidxafter++;
    float p1 = getPDz( dz, evidxafter-1 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float zpos = z0 + idx;

	while ( evidxafter < evposns_.size()-1
	     && evposns_[evidxafter+1] < zpos )
	{
	    evidxafter++;
	    p1 = getPDz( dz, evidxafter-1 );
	}

	float r = (zpos - p1) / dz[1];
	float t = dz[1] + r * dz[2] + (1-r) * dz[0];
	float freq = 1. / (refstep * t);
	setOutputValue( output, 0, idx, z0, freq );
    }
}


void EventFreq::fillPhaseOutput( const DataHolder& output,
				 int z0, int nrsamples ) const
{
    if ( evposns_.size() == 0 )
    {
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, mUdf(float) );
    }
    if ( evposns_.size() == 1 )
    {
	firstevmax_ = !firstevmax_;
	evposns_.insert( 0, z0 );
	evposns_ += z0 + nrsamples - 1;
    }

    int evidxafter = 0;
    float dz[3];
    while ( evidxafter < evposns_.size()-1 && evposns_[evidxafter+1] < z0 )
	evidxafter++;
    float p1 = getPDz( dz, evidxafter-1 );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float zpos = z0 + idx;

	while ( evidxafter < evposns_.size()-1
	     && evposns_[evidxafter+1] < zpos )
	{
	    evidxafter++;
	    p1 = getPDz( dz, evidxafter-1 );
	}

	float r = (zpos - p1) / dz[1];
	float ph = r*M_PI;
	bool oddevnr = evidxafter % 2;
	bool aftermin = (oddevnr && !firstevmax_) || (!oddevnr && firstevmax_);
	if ( aftermin ) ph -= M_PI;
	setOutputValue( output, 1, idx, z0, ph );
    }
}


bool EventFreq::computeData( const DataHolder& output, const BinID& relpos, 
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inpseries_ ) return false;

    evposns_.erase();
    findEvents( z0, nrsamples );
    if ( outputinterest[0] )
	fillFreqOutput( output, z0, nrsamples );
    if ( outputinterest[1] )
	fillPhaseOutput( output, z0, nrsamples );

    return true;
}

}; //namespace
