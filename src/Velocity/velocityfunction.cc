/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: velocityfunction.cc,v 1.6 2010-08-20 03:41:47 cvskris Exp $";

#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "interpol1d.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"

namespace Vel
{

Function::Function( FunctionSource& vfs )
    : source_( vfs )
    , cache_( 0 )
    , desiredrg_( SI().zRange(true).start, SI().zRange(true).stop,
	          SI().zRange(true).step )
    , bid_( mUdf(int),mUdf(int) )
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


float Function::getVelocity( float z )const
{
    Threads::MutexLocker lock( cachelock_ );
    if ( !cache_ )
    {
	const StepInterval<float> sampling = getDesiredZ();
	cachesd_ = sampling;
	const int zstart = mNINT(sampling.start/sampling.step);
	const int zstop = mNINT(sampling.stop/sampling.step);
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ ) return mUdf(float);

	if ( !computeVelocity( cachesd_.start, cachesd_.step,
		    	       cache_->size(), cache_->arr() ) )
	{
	    delete cache_;
	    cache_ = 0;
	    return mUdf( float );
	}
    }

    lock.unLock();

    const float fsample = cachesd_.getIndex( z );
    const int isample = (int) fsample;
    if ( isample<0 || isample>=cache_->size() )
	return mUdf(float);

    return Interpolate::linearReg1DWithUdf(
	    (*cache_)[isample],
	    isample<(cache_->size()-1)
		? (*cache_)[isample+1]
		: mUdf(float),
	    fsample-isample );
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
    Threads::MutexLocker lock( cachelock_ );
    delete cache_; cache_ = 0;
}


mImplFactory1Param( FunctionSource, const MultiID&, FunctionSource::factory );


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	return ioobj->name();
    
    return BufferString( type() );
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


RefMan<const Function> FunctionSource::getFunction( const BinID& bid )
{
    if ( mIsUdf(bid.inl) || mIsUdf(bid.crl) )
	return 0;

    RefMan<const Function> res = 0;
    RefMan<Function> tmpfunc = 0;

    functionslock_.readLock();
    bool iswritelock = false;
    int idx = findFunction( bid );
    if ( idx==-1 )
    {
	functionslock_.readUnLock();

	tmpfunc = createFunction(bid);
	if ( !tmpfunc )
	    return 0;

	functionslock_.writeLock();
	iswritelock = true;
	idx = findFunction( bid );

	if ( idx==-1 )
	{
	    idx = functions_.size();
	    functions_ += tmpfunc;
	}
    }

    if ( idx!=-1 )
	res = functions_[idx];

    if ( iswritelock )
	functionslock_.writeUnLock();
    else
	functionslock_.readUnLock();

    return res;
}


void FunctionSource::removeFunction(Function* v)
{
    functionslock_.writeLock();
    functions_ -= v;
    functionslock_.writeUnLock();
}


}; //namespace




//#include "velocityfunctionsource.h"

