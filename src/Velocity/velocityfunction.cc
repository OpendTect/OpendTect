/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "interpol1d.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "velocitycalc.h"

namespace Vel
{

static Pos::GeomID sGeomID = Survey::default3DGeomID();

Function::Function( FunctionSource& vfs )
    : source_(vfs)
    , bid_(mUdf(int),mUdf(int))
    , desiredrg_(SI().zRange(true))
{
    source_.ref();
    sGeomID = Survey::default3DGeomID();
}


Function::~Function()
{
    removeCache();
    source_.removeFunction( this );
    source_.unRef();
}


const VelocityDesc& Function::getDesc() const
{
    return source_.getDesc();
}


const StepInterval<float>& Function::getDesiredZ() const
{
    return desiredrg_;
}


void Function::setDesiredZRange( const StepInterval<float>& n )
{
    desiredrg_ = n;
}


void Function::setGeomID( const Pos::GeomID& geomid )
{
    sGeomID = geomid;
}


Pos::GeomID Function::getGeomID() const
{
    return sGeomID;
}


#define cDefSampleSnapDist 1e-3f

float Function::getVelocity( float z ) const
{
    Threads::Locker cachelckr( cachelock_ );
    if ( !cache_ )
    {
	const StepInterval<float> sampling( getDesiredZ() );
	cachesd_ = getDoubleSamplingData( SamplingData<float>( sampling ) );
	const int zstart = mNINT32( cachesd_.start / cachesd_.step );
	const int zstop = mNINT32( mCast(double,sampling.stop)/cachesd_.step );
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ ) return mUdf(float);

	if ( !computeVelocity( (float) cachesd_.start, (float) cachesd_.step,
			       cache_->size(), cache_->arr() ) )
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
    {
	return (*cache_)[sampidx];
    }

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
{}


FunctionSource::~FunctionSource()
{}


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	return ioobj->name();

    return BufferString( factoryKeyword() );
}


void FunctionSource::removeFunction( const Function* func )
{
    Threads::Locker lckr( lock_ );

    int idx = functions_.indexOf( func );

    if ( idx!=-1 )
    {
	functions_.removeSingle( idx );
    }
}


const char* FunctionSource::errMsg() const
{ return errmsg_.str(); }


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
	return 0;

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
