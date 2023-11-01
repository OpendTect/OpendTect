/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunctioninterval.h"

#include "survinfo.h"
#include "unitofmeasure.h"
#include "varlenarray.h"
#include "veldescimpl.h"
#include "zvalseriesimpl.h"

namespace Vel
{


IntervalFunction::IntervalFunction( IntervalSource& source )
    : Function(source)
    , inputfunc_(nullptr)
{}


IntervalFunction::~IntervalFunction()
{
    unRefPtr( inputfunc_ );
}


const ZDomain::Info& IntervalFunction::zDomain_() const
{
    return inputfunc_ ? inputfunc_->zDomain() : Function::zDomain();
}


void IntervalFunction::setInput( Function* func )
{
    unRefPtr( inputfunc_ );
    inputfunc_ = func;
    refPtr( inputfunc_ );
}


Function& IntervalFunction::setZDomain_( const ZDomain::Info& )
{
    return *this;
}


bool IntervalFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo(bid) || !inputfunc_->moveTo(bid) )
	return false;

    if ( getDesc().isUdf() )
	copyDescFrom( getSource() );

    return true;
}


StepInterval<float> IntervalFunction::getAvailableZ() const
{
    return inputfunc_->getAvailableZ();
}


bool IntervalFunction::computeVelocity( float z0, float dz, int sz,
					float* res ) const
{
    const SamplingData<float> sd( z0, dz );
    const RegularZValues zvals( sd, sz, inputfunc_->zDomain() );
    for ( od_int64 idx=0; idx<sz; idx++ )
	res[idx] = inputfunc_->getVelocity( zvals[idx] );

    if ( getDesc() == inputfunc_->getDesc() )
	return true;

    const Worker worker( inputfunc_->getDesc(), SI().seismicReferenceDatum(),
			 UnitOfMeasure::surveyDefSRDStorageUnit() );
    ArrayZValues<float> vels( res, sz, zDomain() );
    return worker.convertVelocities( vels.asVS(), zvals, getDesc(),vels.asVS());
}


// IntervalSource

IntervalSource::IntervalSource()
    : inputsource_(nullptr)
    , veldesc_(OD::VelocityType::Interval)

{
    Worker::setUnit( UnitOfMeasure::surveyDefVelStorageUnit(), veldesc_ );
}


IntervalSource::~IntervalSource()
{
    detachAllNotifiers();
    unRefPtr( inputsource_ );
}


const ZDomain::Info& IntervalSource::zDomain_() const
{
    return inputsource_ ? inputsource_->zDomain() : SI().zDomainInfo();
}


const UnitOfMeasure* IntervalSource::getVelUnit_() const
{
    return inputsource_ ? inputsource_->velUnit()
			: UnitOfMeasure::surveyDefVelUnit();
}


const VelocityDesc& IntervalSource::getDesc() const
{
    return veldesc_;
}


void IntervalSource::setInput( FunctionSource* input )
{
    if ( inputsource_ )
    {
	if ( inputsource_->changeNotifier() )
	    mDetachCB( *inputsource_->changeNotifier(),
		       IntervalSource::sourceChangeCB );
	inputsource_->unRef();
    }

    inputsource_ = input;

    if ( inputsource_ )
    {
	inputsource_->ref();
	if ( inputsource_->changeNotifier() )
	    mAttachCB( *inputsource_->changeNotifier(),
		       IntervalSource::sourceChangeCB );
    }
}


void IntervalSource::getAvailablePositions( BinIDValueSet& bidset ) const
{
    if ( inputsource_ )
	inputsource_->getAvailablePositions( bidset );
}


NotifierAccess* IntervalSource::changeNotifier()
{
    return inputsource_ ? inputsource_->changeNotifier() : nullptr;
}


BinID IntervalSource::changeBinID() const
{ return inputsource_->changeBinID(); }


IntervalFunction* IntervalSource::createFunction( const BinID& bid )
{
    if ( !inputsource_ || (!inputsource_->getDesc().isRMS() &&
			   !inputsource_->getDesc().isAvg()) )
	return nullptr;

    RefMan<Function> inputfunc = inputsource_->createFunction( bid );
    if ( !inputfunc )
	return nullptr;

    auto* res = new IntervalFunction( *this );
    res->setInput( inputfunc );

    return res;
}


void IntervalSource::sourceChangeCB( CallBacker* cb )
{
    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    Threads::Locker lock( lock_ );

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( IntervalFunction*, func, functions_[idx] );
	if ( !bid.isUdf() && func->getBinID()!=bid )
	    continue;

	func->removeCache();
    }
}

} // namespace Vel
