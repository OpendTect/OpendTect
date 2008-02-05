/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackagc.cc,v 1.8 2008-02-05 09:24:17 cvsbert Exp $";

#include "prestackagc.h"

#include "dataclipper.h"
#include "flatposdata.h"
#include "iopar.h"
#include "prestackgather.h"
#include "varlenarray.h"


using namespace PreStack;


void AGC::initClass()
{
    PF().addCreator( AGC::createFunc, AGC::sName() );
}


Processor* AGC::createFunc()
{ return new AGC; }


AGC::AGC()
    : Processor( sName() )
    , window_( -100, 100 )
    , mutefraction_( 0 )
{}


bool AGC::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] ) continue;

	const float zstep = inputs_[idx]->posData().range(false).step;

	samplewindow_.start = mNINT( window_.start/zstep );
	samplewindow_.stop = mNINT( window_.stop/zstep );
	return true;
    }

    return false;
}


void AGC::setWindow( const Interval<float>& iv )
{ window_ = iv; }


const Interval<float>& AGC::getWindow() const
{ return window_; }


void AGC::setLowEnergyMute( float iv )
{ mutefraction_ = iv; }


float AGC::getLowEnergyMute() const
{ return mutefraction_; }


void AGC::fillPar( IOPar& par ) const
{
    par.set( sKeyWindow(), window_ );
    par.set( sKeyMuteFraction(), mutefraction_ );
}


bool AGC::usePar( const IOPar& par )
{
    par.get( sKeyWindow(), window_ );
    par.get( sKeyMuteFraction(), mutefraction_ );
    return true;
}


bool AGC::doWork( int start, int stop, int )
{
    const bool doclip = !mIsZero( mutefraction_, 1e-5 );
    DataClipper clipper;

    for ( int offsetidx=start; offsetidx<=stop; offsetidx++, reportNrDone() )
    {
	for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	{
	    Gather* output = outputs_[idx];
	    const Gather* input = inputs_[idx];
	    if ( !output || !input )
		continue;

	    const int nrsamples = input->data().info().getSize( Gather::zDim());
	    
	    mVariableLengthArr( float, energies, nrsamples );
	    for ( int sampleidx=0; sampleidx<nrsamples; sampleidx++ )
	    {
		const float value = input->data().get(offsetidx,sampleidx);
		energies[sampleidx] = mIsUdf( value ) ? value : value*value;
	    }

	    Interval<float> cliprange;
	    if ( doclip )
	    {
		clipper.putData( energies, nrsamples );
		if ( !clipper.calculateRange(mutefraction_,0,cliprange) )
		{
		    //todo: copy old trace
		    continue;
		}
	    }

	    for ( int sampleidx=0; sampleidx<nrsamples; sampleidx++ )
	    {
		int nrenergies = 0;
		float energysum = 0;
		for ( int energyidx=sampleidx+samplewindow_.start;
			  energyidx<=sampleidx+samplewindow_.stop;
			  energyidx++ )
		{
		    if ( energyidx<0 || energyidx>=nrsamples )
			continue;

		    const float energy = energies[energyidx];
		    if ( mIsUdf( energy ) )
			continue;

		    if ( doclip && !cliprange.includes( energy ) )
			continue;
		    
		    energysum += energy;
		    nrenergies++;
		}

		float outputval = 0;
		if ( nrenergies && !mIsZero(energysum,1e-6) )
		{
		    const float inpval = input->data().get(offsetidx,sampleidx);
		    outputval = inpval/sqrt(energysum/nrenergies);
		}

		output->data().set( offsetidx, sampleidx, outputval );
	    }
	}
    }

    return true;
}
