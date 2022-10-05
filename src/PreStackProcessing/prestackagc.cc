/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackagc.h"

#include "agc.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "iopar.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "varlenarray.h"

namespace PreStack
{

const char* AGC::sKeyWindow()		{ return "Window"; }
const char* AGC::sKeyMuteFraction()	{ return "Mutefraction"; }

AGC::AGC()
    : Processor(sFactoryKeyword())
    , window_(-100,100)
{
}


AGC::~AGC()
{
}


bool AGC::prepareWork()
{
    totalnr_ = mCast(int,inputs_.size()*nrIterations());

    if ( !Processor::prepareWork() )
	return false;

    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] ) continue;

	float zstep = (float) inputs_[idx]->posData().range(false).step;
	zstep *= SI().zIsTime() ? 1000 : 1;

	samplewindow_.start = mNINT32( window_.start/zstep );
	samplewindow_.stop = mNINT32( window_.stop/zstep );
	return true;
    }

    return false;
}


void AGC::setWindow( const Interval<float>& iv )
{ window_ = iv; }

const Interval<float>& AGC::getWindow() const
{ return window_; }

void AGC::getWindowUnit( BufferString& buf, bool parens ) const
{ buf = SI().getZUnitString( parens ); }

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


bool AGC::doWork( od_int64 start, od_int64 stop, int )
{
    ::AGC<float> agc;
    agc.setMuteFraction( mutefraction_ );
    agc.setSampleGate( samplewindow_ );

    const int incr = mCast( int, stop-start+1 );
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

	const int lastoffset = input->size( Gather::offsetDim()==0 ) - 1;

	const int curstop = mCast( int, mMIN(lastoffset,stop) );
	for ( int offsetidx=mCast(int,start); offsetidx<=curstop; offsetidx++ )
	{
	    inputtrace.setPos( Gather::offsetDim(), offsetidx );
	    outputtrace.setPos( Gather::offsetDim(), offsetidx );
	    if ( !inputtrace.init() || !outputtrace.init() )
		continue;

	    agc.setInput( inputtrace, inputtrace.info().getSize(0) );
	    agc.setOutput( outputtrace );
	    agc.executeParallel( false );
	}
    }

    return true;
}

} // namespace PreStack
