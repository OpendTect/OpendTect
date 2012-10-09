/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id$";

#include "velocityfunctioninterval.h"

#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{


IntervalFunction::IntervalFunction( IntervalSource& source )
    : Function( source )
    , inputfunc_( 0 )
{}


IntervalFunction::~IntervalFunction()
{
    setInput( 0 );
}


void IntervalFunction::setInput( Function* func )
{
    if ( inputfunc_ ) inputfunc_->unRef();
    inputfunc_ = func;
    if ( inputfunc_ ) inputfunc_->ref();
}


bool IntervalFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    return inputfunc_->moveTo( bid );
}


StepInterval<float> IntervalFunction::getAvailableZ() const
{ return inputfunc_->getAvailableZ(); }


bool IntervalFunction::computeVelocity( float z0, float dz, int nr,
				       float* res ) const
{
    mAllocVarLenArr( float, input, nr );
    if ( !mIsVarLenArrOK(input) ) return false;

    const SamplingData<double> sd( z0, dz );
    
    for ( int idx=0; idx<nr; idx++ )
    {
	float z = sd.atIndex( idx );
	input[idx] = inputfunc_->getVelocity( z );
    }

    return computeDix( input, sd, nr, res );
}


IntervalSource::IntervalSource()
    : inputsource_( 0 )
    , veldesc_( VelocityDesc::Interval )

{}


IntervalSource::~IntervalSource()
{
    setInput( 0 );
}


const VelocityDesc& IntervalSource::getDesc() const
{ return veldesc_; }


void IntervalSource::setInput( FunctionSource* input )
{
    if ( inputsource_ )
    {
	if ( inputsource_->changeNotifier() )
	    inputsource_->changeNotifier()->notify(
		mCB( this, IntervalSource, sourceChangeCB ));
	inputsource_->unRef();
    }

    inputsource_ = input;

    if ( inputsource_ )
    {
	inputsource_->ref();
	if ( inputsource_->changeNotifier() )
	{
	    inputsource_->changeNotifier()->notify(
		mCB( this, IntervalSource, sourceChangeCB ));
	}
    }
}


void IntervalSource::getAvailablePositions( BinIDValueSet& bidset ) const
{
    if ( inputsource_ )
	inputsource_->getAvailablePositions( bidset );
}


NotifierAccess* IntervalSource::changeNotifier()
{ return inputsource_ ? inputsource_->changeNotifier() : 0; }


BinID IntervalSource::changeBinID() const
{ return inputsource_->changeBinID(); }


IntervalFunction* IntervalSource::createFunction( const BinID& bid )
{
    if ( !inputsource_ || inputsource_->getDesc().type_!=VelocityDesc::RMS )
	return 0;

    RefMan<Function> inputfunc = inputsource_->createFunction( bid );
    if ( !inputfunc )
	return 0;

    IntervalFunction* res = new IntervalFunction( *this );
    res->setInput( inputfunc );

    return res;
}


void IntervalSource::sourceChangeCB( CallBacker* cb )
{
    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    Threads::MutexLocker lock( lock_ );

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( IntervalFunction*, func, functions_[idx] );
	if ( bid.inl!=-1 && bid.crl!=-1 && func->getBinID()!=bid )
	    continue;

	func->removeCache();
    }
}




}; //namespace
