/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "dataclipper.h"

#include "arraynd.h"
#include "math2.h"
#include "iopar.h"
#include "sorting.h"
#include "statrand.h"
#include "undefval.h"
#include "valseries.h"
#include "varlenarray.h"
#include <math.h>


DataClipper::DataClipper()
    : sampleprob_( 1 )
    , subselect_( false )
    , approxstatsize_( 2000 )
    , absoluterg_( mUdf(float), -mUdf(float) )
{
    Stats::randGen().init();
} 


void DataClipper::setApproxNrValues( od_int64 n, int statsz )
{
    sampleprob_ = ((float) statsz) / n;
    approxstatsize_ = statsz;

    subselect_ = sampleprob_<1;
    sampleprob_ = mMIN( sampleprob_, 1 );
}

#define mAddValue( array, rg ) \
if ( Math::IsNormalNumber( val ) && !mIsUdf( val ) )  \
{ \
    array += val; \
    rg.include( val, false ); \
}


void DataClipper::putData( float val )
{
    if ( subselect_ )
    {
	double rand = Stats::randGen().get();

	if ( rand>sampleprob_ )
	    return;
    }

    mAddValue( samples_, absoluterg_ );
}

template <class T>
class DataClipperDataInserter : public ParallelTask
{
public:
    DataClipperDataInserter( const T& input, od_int64 sz, LargeValVec<float>& samples,
			     Interval<float>& rg, float prob )
        : input_( input )
        , nrvals_( sz )
        , samples_( samples )
        , doall_( mIsEqual( prob, 1, 1e-3 ) )
	, absoluterg_( rg )
    {
	nrsamples_ = doall_ ? nrvals_ : mNINT64(sz * prob);
    }
    
    od_int64 nrIterations() const
    { return nrsamples_; }
    
    int minThreadSize() const { return 100000; }
    
    bool doWork( od_int64 start, od_int64 stop, int )
    {
	TypeSet<float> localsamples;
	Interval<float> localrg( mUdf(float), -mUdf(float) );
	
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    float val;
	    if ( doall_ )
	    {
		val = input_[idx];
	    }
	    else
	    {
		double rand = Stats::randGen().get();
		rand *= (nrvals_-1);
		const od_int64 sampidx = mNINT64(rand);
		val = input_[sampidx];
	    }
	    
	    
	    mAddValue( localsamples, localrg );
	}
	
	if ( localsamples.size() )
	{
	    Threads::SpinLockLocker lock( lock_ );
	   
	    append( samples_, localsamples );
	    absoluterg_.include( localrg, false );
	}
	
	return true;
    }
    
protected:
    
    Threads::SpinLock	lock_;
    od_int64		nrsamples_;
    od_int64		nrvals_;
    LargeValVec<float>&	samples_;
    const T&		input_;
    bool		doall_;
    Interval<float>&	absoluterg_;
};


void DataClipper::putData( const float* vals, od_int64 nrvals )
{
    DataClipperDataInserter<const float*> inserter( vals, mCast(int,nrvals),
					samples_, absoluterg_,sampleprob_ );
    
    inserter.execute();
}


void DataClipper::putData( const ValueSeries<float>& vals, od_int64 nrvals )
{
    if ( vals.arr() )
    {
	putData( vals.arr(), nrvals );
	return;
    }
    
    DataClipperDataInserter<const ValueSeries<float> > inserter( vals, 
			mCast(int,nrvals), samples_, absoluterg_, sampleprob_ );
    
    inserter.execute();
}


void DataClipper::putData( const ArrayND<float>& vals )
{
    const od_int64 nrvals = vals.info().getTotalSz();
    if ( vals.getStorage() )
    {
	putData( *vals.getStorage(), nrvals );
	return;
    }
    
    ArrayNDValseriesAdapter<float> adapter( vals );
    if ( !adapter.isOK() )
    {
	pErrMsg("Problem with adapter");
	return;
    }
    
    putData( adapter, vals.info().getTotalSz() );
}


bool DataClipper::calculateRange( float cliprate, Interval<float>& range )
				  
{
    return calculateRange( cliprate, cliprate, range );
}


bool DataClipper::calculateRange( float* vals, od_int64 nrvals,
				  float lowcliprate, float highcliprate,
				  Interval<float>& range )
{
    if ( !nrvals ) return false;

    od_int64 firstidx = mNINT64(lowcliprate*nrvals);
    od_int64 topnr = mNINT64(highcliprate*nrvals);
    od_int64 lastidx = nrvals-topnr-1;

    if ( firstidx && topnr )
    {
	sortFor( vals, mCast(int,nrvals), mCast(int,firstidx) );
	range.start = vals[firstidx];

	sortFor( vals, mCast(int,nrvals), mCast(int,lastidx) );
	range.stop = vals[lastidx];
    }
    else
    {
	float min=mUdf(float), max=mUdf(float);
	bool isset = false;
	for ( od_int64 idx=0; idx<nrvals; idx++ )
	{
	    const float val = vals[idx];

	    if ( mIsUdf(val) )
		continue;

	    if ( !isset || min>val )
		min = val;

	    if ( !isset || max<val )
		max = val;

	    isset = true;
	}

	if ( !isset )
	    return false;

	range.start = min;
	range.stop = max;
    }

    return true;
}


bool DataClipper::calculateRange( float lowcliprate, float highcliprate,
				  Interval<float>& range )
				  
{
    const bool res = calculateRange( samples_.arr(), samples_.size(),
	    lowcliprate, highcliprate, range );

    reset();

    return res;
}


bool DataClipper::fullSort()
{
    od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    if ( nrvals>100 )
	quickSort( samples_.arr(), mCast(int,nrvals) );
    else
	sort_array( samples_.arr(), mCast(int,nrvals) );

    return true;
}


bool DataClipper::getRange( float cliprate, Interval<float>& range ) const
{
    return getRange( cliprate/2, cliprate/2, range );
}


bool DataClipper::getRange( float lowclip, float highclip,
			    Interval<float>& range ) const
{
    if ( lowclip>1 || highclip>1 || highclip+lowclip>1 )
    {
	pErrMsg("The cliprate should between 0 and 1");
	return false;
    }

    od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;
    
    if ( mIsZero(lowclip, 1e-5 ) )
    {
	range.start = absoluterg_.start;
    }
    else
    {
	const od_int64 firstidx = mNINT64(lowclip*nrvals);
	range.start = samples_[ mCast(int,firstidx) ];
    }
    
    if ( mIsZero( highclip, 1e-5 ) )
    {
	range.stop = absoluterg_.stop;
    }
    else
    {
	const od_int64 topnr = mNINT64(highclip*nrvals);
	const od_int64 lastidx = nrvals-topnr-1;
	
	range.stop = samples_[ mCast(int,lastidx) ];
    }
    
    return true;
}


bool DataClipper::getSymmetricRange( float cliprate, float midval,
				     Interval<float>& range ) const
{
    const od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    const od_int64 nrsamplestoremove = mNINT64(cliprate*nrvals);

    od_int64 firstsample = 0;
    od_int64 lastsample = nrvals-1;

    for ( od_int64 idx=0; idx<nrsamplestoremove; idx++ )
    {
	if ( firstsample==lastsample )
	    break;

	const float firstdist = fabs(midval-samples_[mCast(int,firstsample)]);
	const float lastdist = fabs(midval-samples_[mCast(int,lastsample)]);

	if ( firstdist>lastdist )
	    firstsample++;
	else
	    lastsample--;
    }


    const float firstdist = fabs(midval-samples_[mCast(int,firstsample)]);
    const float lastdist = fabs(midval-samples_[mCast(int,lastsample)]);
    const float halfwidth = mMAX( firstdist, lastdist );

    range.start = midval-halfwidth;
    range.stop = midval+halfwidth;

    return true;
}


void DataClipper::reset()
{
    samples_.erase();
    subselect_ = false;
    sampleprob_ = 1;
    absoluterg_.start = mUdf(float);
    absoluterg_.stop = -mUdf(float);
}


DataClipSampler::DataClipSampler( int ns )
    : maxnrvals_(ns)
    , vals_(new float [ns])
    , count_(0)
    , finished_(false)
{
}


void DataClipSampler::add( const float* v, od_int64 sz )
{
    if ( count_ < maxnrvals_ )
    {
	for ( int idx=0; idx<sz; idx++ )
	    doAdd( v[idx] );
	return;
    }

    const float relwt = maxnrvals_ / ((float)count_);
    const int nr2add = (int)(relwt * sz - .5);
    if ( nr2add < 1 ) return;

    od_int64 randint = Stats::randGen().getIndex( mUdf(int) );
    for ( int idx=0; idx<nr2add; idx++ )
    {
	od_int64 vidx = Stats::randGen().getIndexFast( sz, randint );
	doAdd( v[vidx] );
	randint *= mCast( int, vidx );
    }
}


void DataClipSampler::add( float val )
{
    if ( Stats::randGen().getIndex(count_) < maxnrvals_ )
	doAdd( val );
}


void DataClipSampler::doAdd( float val )
{
    if ( !Math::IsNormalNumber(val) || mIsUdf(val) || val > 1e32 || val < -1e32)
	return;

    if ( finished_ )
	finished_ = false;

    if ( count_ < maxnrvals_ )
	vals_[count_] = val;
    else
	vals_[ Stats::randGen().getIndex(maxnrvals_) ] = val;

    count_++;
}

od_int64 DataClipSampler::nrVals() const
{ return mCast( int, count_ > maxnrvals_ ? maxnrvals_ : count_ ); }


void DataClipSampler::finish() const
{
    DataClipSampler& self = *const_cast<DataClipSampler*>(this);
    sort_array( self.vals_, nrVals() );
    self.finished_ = true;
}


Interval<float> DataClipSampler::getRange( float clip ) const
{
    if ( !finished_ ) finish();

    const od_int64 nv = nrVals();
    if ( nv == 0 ) return Interval<float>(0,0);

    const float fidx = nv * .5f * clip;
    od_int64 idx0 = mNINT64(fidx);
    od_int64 idx1 = nv - idx0 - 1;
    if ( idx0 > idx1 ) Swap( idx0, idx1 );

    return Interval<float>( vals_[idx0], vals_[idx1] );
}


const char* DataClipSampler::getClipRgStr( float pct ) const
{
    Interval<float> rg( getRange(pct * 0.01f) );
    static BufferString ret;
    ret = rg.start; ret += " - "; ret += rg.stop;

    float maxabs = fabs( rg.start );
    if ( fabs(rg.stop) > maxabs ) maxabs = fabs( rg.stop );
    if ( maxabs != 0 ) 
    {
	const float sc8 = 127 / maxabs;
	const float sc16 = 32767 / maxabs;
	ret += " [scl 16/8-bits: "; ret += sc16;
	ret += " ; "; ret += sc8; ret += "]";
    }

    return ret.buf();
}


void DataClipSampler::report( IOPar& iop ) const
{
    if ( nrVals() < 3 )
	iop.add( "Not enough values collected", nrVals() );
    else
    {
	iop.add( "Value range", getClipRgStr(0) );
	iop.add( "0.1% clipping range", getClipRgStr(0.1) );
	iop.add( "0.25% clipping range", getClipRgStr(0.25) );
	iop.add( "0.5% clipping range", getClipRgStr(0.5) );
	iop.add( "1% clipping range", getClipRgStr(1) );
	iop.add( "2.5% clipping range", getClipRgStr(2.5) );
	iop.add( "5% clipping range", getClipRgStr(5) );
	iop.add( "10% clipping range", getClipRgStr(10) );
	iop.add( "25% clipping range", getClipRgStr(25) );
	iop.add( "Median value", vals_[nrVals()/2] );
    }
}
