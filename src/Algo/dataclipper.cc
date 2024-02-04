/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "dataclipper.h"

#include "arraynd.h"
#include "atomic.h"
#include "math2.h"
#include "iopar.h"
#include "simpnumer.h"
#include "sorting.h"
#include "statrand.h"
#include "undefval.h"
#include "valseries.h"
#include "varlenarray.h"
#include <math.h>


DataClipper::DataClipper()
    : gen_(*new Stats::RandGen())
    , absoluterg_( mUdf(float), -mUdf(float) )
{
}


DataClipper::DataClipper( const DataClipper& oth )
    : gen_(*new Stats::RandGen())
{
    *this = oth;
}


DataClipper::~DataClipper()
{
    delete &gen_;
}


DataClipper& DataClipper::operator =( const DataClipper& oth )
{
    if ( &oth != this )
    {
	approxstatsize_ = oth.approxstatsize_;
	sampleprob_ = oth.sampleprob_;
	subselect_ = oth.subselect_;
	samples_ = oth.samples_;
	absoluterg_ = oth.absoluterg_;
    }

    return *this;
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
	if ( gen_.get() > sampleprob_ )
	    return;
    }

    mAddValue( samples_, absoluterg_ );
}

template <class T>
class DataClipperDataInserter : public ParallelTask
{
public:
    DataClipperDataInserter( const T& input, od_int64 sz,
			     LargeValVec<float>& samples,
			     Interval<float>& rg, float prob )
        : input_( input )
        , nrvals_( sz )
        , samples_( samples )
        , doall_( mIsEqual( prob, 1, 1e-3 ) )
	, absoluterg_( rg )
    {
	nrsamples_ = doall_ ? nrvals_ : mNINT64(sz * prob);
    }

    od_int64 nrIterations() const override	{ return nrsamples_; }

    int minThreadSize() const override		{ return 100000; }

    bool doWork( od_int64 start, od_int64 stop, int ) override
    {
	TypeSet<float> localsamples;
	Interval<float> localrg( mUdf(float), -mUdf(float) );
	Stats::RandGen gen;
	const od_int64 sz = nrvals_ -1;
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    float val;
	    if ( doall_ )
	    {
		val = input_[idx];
	    }
	    else
	    {
		const od_int64 sampidx = gen.getIndex( sz );
		val = input_[sampidx];
	    }


	    mAddValue( localsamples, localrg );
	}

	if ( localsamples.size() )
	{
	    lock_.lock();
	    append( samples_, localsamples );
	    absoluterg_.include( localrg, false );
	    lock_.unLock();
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
    DataClipperDataInserter<const float*> inserter( vals, nrvals,
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

    DataClipperDataInserter<const ValueSeries<float> > inserter( vals, nrvals,
					samples_, absoluterg_, sampleprob_ );

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
	{ pErrMsg("Problem with adapter"); return; }

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
	sortFor( vals, nrvals, firstidx );
	range.start = vals[firstidx];

	sortFor( vals, nrvals, lastidx );
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

    if ( nrvals<=100 )
    {
	sort_array( samples_.arr(), nrvals );
	return true;
    }

    if ( nrvals>255 && hasDuplicateValues(samples_.arr(),nrvals,256,128) )
    {
	if ( duplicate_sort(samples_.arr(),nrvals,256) )
	    return true;
    }

    quickSort( samples_.arr(), nrvals );
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
	{ pErrMsg("Invalid clip rate passed"); return false; }

    od_int64 nrvals = samples_.size();
    if ( !nrvals ) return false;

    if ( mIsZero(lowclip, 1e-5 ) )
	range.start = absoluterg_.start;
    else
    {
	const od_int64 firstidx = mNINT64(lowclip*nrvals);
	range.start = samples_.validIdx( firstidx) ? samples_[ firstidx ]
						   : samples_[ 0 ];
    }

    if ( mIsZero( highclip, 1e-5 ) )
	range.stop = absoluterg_.stop;
    else
    {
	const od_int64 topnr = mNINT64(highclip*nrvals);
	const od_int64 lastidx = nrvals-topnr-1;

	range.stop = samples_.validIdx( lastidx ) ? samples_[ lastidx ]
						  : samples_[ nrvals-1 ];
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

	const float firstdist = fabs( midval - samples_[firstsample] );
	const float lastdist = fabs( midval - samples_[lastsample] );

	if ( firstdist>lastdist )
	    firstsample++;
	else
	    lastsample--;
    }


    const float firstdist = fabs( midval - samples_[firstsample] );
    const float lastdist = fabs( midval - samples_[lastsample] );
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
    , rg_(mUdf(float),0)
    , gen_(*new Stats::RandGen())
{
}


DataClipSampler::DataClipSampler(const DataClipSampler& oth )
    : gen_(*new Stats::RandGen())
    , vals_(nullptr)
{
    *this = oth;
}


DataClipSampler::~DataClipSampler()
{
    delete [] vals_;
    delete &gen_;
}


DataClipSampler& DataClipSampler::operator=( const DataClipSampler& oth )
{
    if ( &oth != this )
    {
	maxnrvals_ = oth.maxnrvals_;
	delete [] vals_;
	vals_ = new float[maxnrvals_];
	OD::memCopy( vals_, oth.vals_, maxnrvals_*sizeof(float) );
	rg_.setFrom( oth.rg_ );
    }

    return *this;
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

    for ( int idx=0; idx<nr2add; idx++ )
    {
	const od_int64 vidx = gen_.getIndex( sz );
	doAdd( v[vidx] );
    }
}


void DataClipSampler::add( float val )
{
    if ( gen_.getIndex(count_) < maxnrvals_ )
	doAdd( val );
}


void DataClipSampler::doAdd( float val )
{
    if ( !Math::IsNormalNumber(val) || mIsUdf(val) || val > 1e32 || val < -1e32)
	return;

    if ( finished_ )
	finished_ = false;

    if ( mIsUdf(rg_.start) )
	rg_.start = rg_.stop = val;
    else if ( rg_.start > val )
	rg_.start = val;
    else if ( rg_.stop < val )
	rg_.stop = val;

    if ( count_ < maxnrvals_ )
	vals_[count_] = val;
    else
	vals_[ gen_.getIndex(maxnrvals_) ] = val;

    count_++;
}

od_int64 DataClipSampler::nrVals() const
{ return count_ > maxnrvals_ ? maxnrvals_ : count_; }


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

    if ( mIsZero(clip,1.e-6f) && !mIsUdf(rg_.start) )
	return rg_;

    const float fidx = nv * .5f * clip;
    od_int64 idx0 = mNINT64(fidx);
    od_int64 idx1 = nv - idx0 - 1;
    if ( idx0 > idx1 ) Swap( idx0, idx1 );

    return Interval<float>( vals_[idx0], vals_[idx1] );
}


BufferString DataClipSampler::getClipRgStr( float pct ) const
{
    Interval<float> rg( getRange(pct * 0.01f) );
    BufferString ret;
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
