/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binnedvalueset.h"
#include "interpol1d.h"
#include "ioobj.h"
#include "survinfo.h"
#include "velocitycalc.h"

namespace Vel
{

Function::Function( FunctionSource& vfs )
    : source_(vfs)
    , cache_(0)
    , desiredrg_(SI().zRange())
    , bid_(mUdf(int),mUdf(int))
{
    source_.ref();
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
{ return desiredrg_; }


void Function::setDesiredZRange( const StepInterval<float>& n )
{ desiredrg_ = n; }


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
	    deleteAndZeroPtr( cache_ );
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
    delete cache_; cache_ = 0;
}


mImplClassFactory( FunctionSource, factory );


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = mid_.getIOObj();
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


const uiString FunctionSource::errMsg() const
{ return errmsg_; }


void FunctionSource::getSurroundingPositions( const BinID& bid,
					      BinnedValueSet& bids) const
{
    BinnedValueSet mybids( 0, false );
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
    RefMan<Function> tmpfunc = 0;
    int idx = findFunction( bid );
    if ( idx==-1 )
    {
	tmpfunc = createFunction( bid );
	if ( !tmpfunc )
	    return 0;

	functions_ += tmpfunc.ptr();
    }
    else
	tmpfunc = functions_[idx];

    lckr.unlockNow();

    return ConstRefMan<Function>( tmpfunc.ptr() );

}

} // namespace Vel
