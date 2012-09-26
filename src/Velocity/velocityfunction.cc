/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    source_.unRef();
}


void Function::ref() const
{
    source_.refFunction( this );
}


void Function::unRef() const
{
    if ( source_.unRefFunction( this ) )
	delete const_cast<Function*>(this);
}


void Function::unRefNoDelete() const
{
    source_.unRefFunction( this );
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
	const int zstart = mNINT32(sampling.start/sampling.step);
	const int zstop = mNINT32(sampling.stop/sampling.step);
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ ) return mUdf(float);

	if ( !computeVelocity( (float) cachesd_.start, (float) cachesd_.step,
		    	       cache_->size(), cache_->arr() ) )
	{
	    delete cache_;
	    cache_ = 0;
	    return mUdf( float );
	}
    }

    lock.unLock();

    const float fsample = cachesd_.getfIndex( z );
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
    
    return BufferString( factoryKeyword() );
}


void FunctionSource::refFunction( const Function* func )
{
    Threads::MutexLocker lock( lock_ );
    int idx = functions_.indexOf( func );
    if ( idx==-1 )
    {
	idx = refcounts_.size();
	functions_ += const_cast<Function*>( func );
	refcounts_ += 0;
    }
    
    refcounts_[idx]++;
}


bool FunctionSource::unRefFunction( const Function* func )
{
    bool remove = false;
    Threads::MutexLocker lock( lock_ );
    int idx = functions_.indexOf( func );
    if ( idx==-1 )
    {
	pErrMsg("Unknown function" );
    }
    else
    {
	refcounts_[idx]--;
	remove = !refcounts_[idx];

	if ( remove )
	{
	    refcounts_.remove( idx );
	    functions_.remove( idx );
	}
    }

    return remove;
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

    Threads::MutexLocker lock( lock_ );
    Function* tmpfunc = 0;
    int idx = findFunction( bid );  
    if ( idx==-1 )
    {
 	tmpfunc = createFunction( bid );
	if ( !tmpfunc )
	    return 0;

 	functions_ += tmpfunc;
 	refcounts_ += 1;
    }
    else
    {
       	tmpfunc = functions_[idx];
       	refcounts_[idx]++;
    }
    
    lock.unLock();
    
    RefMan<const Function> res = tmpfunc;
    tmpfunc->unRef();
    
    return res;
}


}; //namespace




//#include "velocityfunctionsource.h"

