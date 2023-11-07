/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binidvalset.h"
#include "interpol1d.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"
#include "veldesc.h"
#include "zvalseriesimpl.h"


namespace Vel
{

Function::Function( FunctionSource& vfs )
    : source_(vfs)
    , bid_(mUdf(int),mUdf(int))
    , geomid_(Survey::default3DGeomID())
    , desiredrg_(SI().zRange(true))
    , desc_(*new VelocityDesc)
    , zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{
    desc_.setUnit( UnitOfMeasure::surveyDefVelUnit() );
    source_.ref();
}


Function::~Function()
{
    delete cache_;
    source_.removeFunction( this );
    source_.unRef();
    delete &desc_;
    delete zdomaininfo_;
}


bool Function::zIsTime() const
{
    return zDomain().isTime();
}


bool Function::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool Function::zInFeet() const
{
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* Function::velUnit() const
{
    return getDesc().getUnit();
}


const ZSampling& Function::getDesiredZ() const
{
    return desiredrg_;
}


Function& Function::setDesiredZRange( const ZSampling& zsamp )
{
    desiredrg_ = zsamp;
    return *this;
}


Function& Function::setGeomID( const Pos::GeomID& geomid )
{
    geomid_ = geomid;
    return *this;
}


Function& Function::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );
    desiredrg_.setUdf();

    return *this;
}


Function& Function::copyDescFrom( const FunctionSource& src )
{
    if ( src.getDesc().isUdf() )
	return *this;

    const UnitOfMeasure* veluom = getDesc().getUnit();
    desc_ = src.getDesc();
    desc_.setUnit( veluom );

    return *this;
}


#define cDefSampleSnapDist 1e-3f

float Function::getVelocity( float z ) const
{
    Threads::Locker cachelckr( cachelock_ );
    if ( !cache_ )
    {
	const ZSampling sampling( getDesiredZ() );
	cachesd_ = RegularZValues::getDoubleSamplingData(
					SamplingData<float>( sampling ) );
	const int zstart = mNINT32( cachesd_.start / cachesd_.step );
	const int zstop = mNINT32( mCast(double,sampling.stop)/cachesd_.step );
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ )
	    return mUdf(float);

	if ( !computeVelocity((float)cachesd_.start,(float)cachesd_.step,
			      cache_->size(),cache_->arr()) )
	{
	    deleteAndNullPtr( cache_ );
	    return mUdf( float );
	}
    }
    cachelckr.unlockNow();

    const int sz = cache_->size();
    const int sampidx = cachesd_.nearestIndex( z );
    if ( sampidx<0 || sampidx>=sz )
	return mUdf(float);
    else if ( sampidx<0 )
	return (*cache_)[0];
    else if ( sampidx>=sz-1 )
	return (*cache_)[sz-1];

    const float pos = mCast(float,( z - cachesd_.start ) / cachesd_.step);
    if ( sampidx-pos > -cDefSampleSnapDist && sampidx-pos < cDefSampleSnapDist )
	return (*cache_)[sampidx];

    return Interpolate::linearReg1DWithUdf( (*cache_)[sampidx], sampidx<(sz-1)
					  ? (*cache_)[sampidx+1]
					  : mUdf(float), pos );
}


const BinID& Function::getBinID() const
{ return bid_; }


bool Function::moveTo( const BinID& bid )
{
    bid_ = bid;
    removeCache();

    return true;
}


void Function::removeCache()
{
    Threads::Locker lckr( cachelock_ );
    deleteAndNullPtr( cache_ );
}


// FunctionSource

mImplFactory1Param( FunctionSource, const MultiID&, FunctionSource::factory );


FunctionSource::FunctionSource()
    : zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{}


FunctionSource::~FunctionSource()
{
    delete zdomaininfo_;
}


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	return ioobj->name();

    return BufferString( factoryKeyword() );
}


bool FunctionSource::zIsTime() const
{
    return zDomain().isTime();
}


bool FunctionSource::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool FunctionSource::zInFeet() const
{
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* FunctionSource::velUnit() const
{
    return getDesc().getUnit();
}


FunctionSource& FunctionSource::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );

    return *this;
}


void FunctionSource::removeFunction( const Function* func )
{
    Threads::Locker lckr( lock_ );

    int idx = functions_.indexOf( func );

    if ( idx!=-1 )
	functions_.removeSingle( idx );
}


const char* FunctionSource::errMsg() const
{
    return errmsg_.buf();
}


void FunctionSource::getSurroundingPositions( const BinID& bid,
					      BinIDValueSet& bids) const
{
    BinIDValueSet mybids( 0, false );
    getAvailablePositions( mybids );
    if ( !mybids.isEmpty() )
	bids.append( mybids ); //Filter?
}


int FunctionSource::findFunction( const BinID& bid ) const
{
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	if ( functions_[idx]->getBinID()==bid )
	    return idx;
    }

    return -1;
}


ConstRefMan<Function> FunctionSource::getFunction( const BinID& bid )
{
    if ( mIsUdf(bid.inl()) || mIsUdf(bid.crl()) )
	return nullptr;

    Threads::Locker lckr( lock_ );
    RefMan<Function> tmpfunc;
    int idx = findFunction( bid );
    if ( idx==-1 )
    {
	tmpfunc = createFunction( bid );
	if ( !tmpfunc )
	    return nullptr;

	functions_ += tmpfunc.ptr();
    }
    else
	tmpfunc = functions_[idx];

    lckr.unlockNow();

    return ConstRefMan<Function>( tmpfunc.ptr() );
}

} // namespace Vel
