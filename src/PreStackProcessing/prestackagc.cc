/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackagc.cc,v 1.20 2012/07/10 13:06:02 cvskris Exp $";

#include "prestackagc.h"

#include "agc.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "iopar.h"
#include "prestackgather.h"
#include "varlenarray.h"
#include "survinfo.h"


PreStack::AGC::AGC()
    : Processor( sFactoryKeyword() )
    , window_( -100, 100 )
    , mutefraction_( 0 )
    , totalnr_( -1 )
{}


bool PreStack::AGC::prepareWork()
{
    totalnr_ = inputs_.size()*nrIterations();

    if ( !Processor::prepareWork() )
	return false;

    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] ) continue;

	float zstep = inputs_[idx]->posData().range(false).step;
	zstep *= SI().zIsTime() ? 1000 : 1;

	samplewindow_.start = mNINT32( window_.start/zstep );
	samplewindow_.stop = mNINT32( window_.stop/zstep );
	return true;
    }

    return false;
}


void PreStack::AGC::setWindow( const Interval<float>& iv )
{ window_ = iv; }


const Interval<float>& PreStack::AGC::getWindow() const
{ return window_; }


void PreStack::AGC::getWindowUnit( BufferString& buf, bool parens ) const
{
    buf = SI().getZUnitString( parens );
}


void PreStack::AGC::setLowEnergyMute( float iv )
{ mutefraction_ = iv; }


float PreStack::AGC::getLowEnergyMute() const
{ return mutefraction_; }


void PreStack::AGC::fillPar( IOPar& par ) const
{
    par.set( sKeyWindow(), window_ );
    par.set( sKeyMuteFraction(), mutefraction_ );
}


bool PreStack::AGC::usePar( const IOPar& par )
{
    par.get( sKeyWindow(), window_ );
    par.get( sKeyMuteFraction(), mutefraction_ );
    return true;
}


bool PreStack::AGC::doWork( od_int64 start, od_int64 stop, int )
{
    ::AGC<float> agc;
    agc.setMuteFraction( mutefraction_ );
    agc.setSampleGate( samplewindow_ );

    const int incr = stop-start+1;
    for ( int idx=outputs_.size()-1; idx>=0; idx--, addToNrDone(incr) )
    {
	Gather* output = outputs_[idx];
	const Gather* input = inputs_[idx];
	if ( !output || !input )
	    continue;

	Array1DSlice<float> inputtrace( input->data() );
	inputtrace.setDimMap( 0, Gather::zDim() );

	Array1DSlice<float> outputtrace( output->data() );
	outputtrace.setDimMap( 0, Gather::zDim() );

	const int lastoffset = input->size( Gather::offsetDim()==0 ) -1;

	const int curstop = mMIN(lastoffset,stop);
	for ( int offsetidx=start; offsetidx<=curstop; offsetidx++ )
	{
	    inputtrace.setPos( Gather::offsetDim(), offsetidx );
	    outputtrace.setPos( Gather::offsetDim(), offsetidx );

	    if ( !inputtrace.init() || !outputtrace.init() )
		continue;

	    agc.setInput( inputtrace, inputtrace.info().getSize(0) );
	    agc.setOutput( outputtrace );
	    agc.execute( false );
	}
    }

    return true;
}
