/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR	: Y.C. Liu
 * DATE:	March 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "volprocthresholder.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "iopar.h"
#include "volumeprocessing.h"

using namespace VolProc;


void ThresholdStep::initClass()
{
    VolProc::PS().addCreator( create, ThresholdStep::sKeyType() );
}


ThresholdStep::ThresholdStep( ProcessingChain& pc )
    : ProcessingStep( pc )
    , threshold_( 0 )
{}


void ThresholdStep::setThreshold( float nt )
{ threshold_ = nt; }


float ThresholdStep::getThreshold() const
{ return threshold_; }


const char* ThresholdStep::type() const
{ return sKeyType(); }


bool ThresholdStep::needsInput( const HorSampling& ) const
{ return true; }


bool ThresholdStep::compute( int start, int stop )
{
    if ( !input_ || !output_ || !input_->nrCubes() ) return false;

    const StepInterval<int> outputinlrg( output_->inlsampling.start, 
	   		output_->inlsampling.atIndex( output_->getInlSz()-1 ),
	   	 	output_->inlsampling.step );
    
    if ( !outputinlrg.includes( curbid_.inl ) ||
	 (curbid_.inl-outputinlrg.start)%outputinlrg.step )
    return false;

    const StepInterval<int> outputcrlrg( output_->crlsampling.start,
			output_->crlsampling.atIndex( output_->getCrlSz()-1 ),
			output_->crlsampling.step );
    
    if ( !outputcrlrg.includes( curbid_.crl ) ||
	 (curbid_.crl-outputcrlrg.start)%outputcrlrg.step )
    return false;

    const StepInterval<int> inputinlrg( input_->inlsampling.start,
			input_->inlsampling.atIndex( input_->getInlSz()-1 ),
			input_->inlsampling.step );

    if ( !inputinlrg.includes( curbid_.inl ) ||
	 (curbid_.inl-inputinlrg.start)%inputinlrg.step )
    return false;

    const StepInterval<int> inputcrlrg( input_->crlsampling.start,
			input_->crlsampling.atIndex( input_->getCrlSz()-1 ),
			input_->crlsampling.step );
    
    if ( !inputcrlrg.includes( curbid_.crl ) ||
	 (curbid_.crl-inputcrlrg.start)%inputcrlrg.step )
    return false;

    const int inputinlidx = inputinlrg.nearestIndex( curbid_.inl );
    const int inputcrlidx = inputcrlrg.nearestIndex( curbid_.crl );
    const int outputinlidx = outputinlrg.nearestIndex( curbid_.inl );
    const int outputcrlidx = outputcrlrg.nearestIndex( curbid_.crl );

    const Array3D<float>& inputarray = input_->getCube(0);
    if ( !output_->nrCubes() )
    {
	if ( !output_->addCube( 0, false ) ) 
	return false;
    }
    
    Array3D<float>& outputarray = output_->getCube(0);

    for ( int idx=start; idx<=stop; idx++ )
    {
	const float value = inputarray.get( inputinlidx, inputcrlidx, idx );
	if ( mIsUdf(value) )
	    outputarray.set( outputinlidx, outputcrlidx, idx, mUdf(float) );
	else 
	    outputarray.set( outputinlidx, outputcrlidx, idx, value>threshold_);
    }

    return true;
}

void ThresholdStep::fillPar( IOPar& par ) const
{
    ProcessingStep::fillPar( par );

    par.set( sKeyThreshold(), threshold_ );
}


bool ThresholdStep::usePar( const IOPar& par )
{
    if ( !ProcessingStep::usePar( par ) )
	return false;

   return par.get( sKeyThreshold(), threshold_ );
}


ProcessingStep* ThresholdStep::create(ProcessingChain& pc)
{ return new ThresholdStep( pc ); }
