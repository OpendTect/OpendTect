/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: velocityfunctioninterval.cc,v 1.1 2008-12-18 21:39:10 cvskris Exp $";

#include "velocityfunctioninterval.h"

#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{


IntervalFunction::IntervalFunction( IntervalSource& source )
    : Function( source )
    , input_( 0 )
{}


IntervalFunction::~IntervalFunction()
{
    setInput( 0 );
}


void IntervalFunction::setInput( Function* func )
{
    if ( input_ ) input_->unRef();
    input_ = func;
    if ( input_ ) input_->ref();
}


bool IntervalFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    return input_->moveTo( bid );
}


StepInterval<float> IntervalFunction::getAvailableZ() const
{ return input_->getAvailableZ(); }


bool IntervalFunction::computeVelocity( float z0, float dz, int nr,
				       float* res ) const
{
    mAllocVarLenArr( float, input, nr );
    if ( !input ) return false;

    const SamplingData<double> sd( z0, dz );
    
    for ( int idx; idx<nr; idx++ )
    {
	float z = sd.atIndex( idx );
	input[idx] = input_->getVelocity( z );
    }

    return computeDix( input, sd, nr, source_.getDesc().samplespan_, res );
}


IntervalSource::IntervalSource()
    : input_( 0 )
    , veldesc_( VelocityDesc::Interval, VelocityDesc::Above  )

{}


IntervalSource::~IntervalSource()
{
    setInput( 0 );
}


const VelocityDesc& IntervalSource::getDesc() const
{ return veldesc_; }


void IntervalSource::setInput( FunctionSource* input )
{
    if ( !input || input->getDesc().type_!=VelocityDesc::RMS )
	return;

    if ( input_ )
    {
	if ( input_->changeNotifier() )
	    input_->changeNotifier()->notify(
		mCB( this, IntervalSource, sourceChangeCB ));
	input_->unRef();
    }

    input_ = input;

    if ( input_ )
    {
	input_->ref();
	if ( input_->changeNotifier() )
	{
	    input_->changeNotifier()->notify(
		mCB( this, IntervalSource, sourceChangeCB ));
	}

	veldesc_.samplespan_ = input_->getDesc().samplespan_;
    }
}


void IntervalSource::getAvailablePositions( HorSampling& hrg ) const
{
    if ( input_ )
	input_->getAvailablePositions( hrg );
}


void IntervalSource::getAvailablePositions( BinIDValueSet& bidset ) const
{
    if ( input_ )
	input_->getAvailablePositions( bidset );
}


NotifierAccess* IntervalSource::changeNotifier()
{ return input_ ? input_->changeNotifier() : 0; }


BinID IntervalSource::changeBinID() const
{ return input_->changeBinID(); }


IntervalFunction* IntervalSource::createFunction( const BinID& bid )
{
    RefMan<Function> input = input_->createFunction( bid );
    if ( !input )
	return 0;

    IntervalFunction* res = new IntervalFunction( *this );
    res->ref();
    res->setInput( input );

    res->unRefNoDelete();
    return res;
}


void IntervalSource::sourceChangeCB( CallBacker* cb )
{
    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( IntervalFunction*, func, functions_[idx] );
	if ( bid.inl!=-1 && bid.crl!=-1 && func->getBinID()!=bid )
	    continue;

	func->removeCache();
    }
}




}; //namespace
