#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2017
________________________________________________________________________


-*/

#include "datadistribution.h"
#include "arraynd.h"
#include "valseries.h"
#include "paralleltask.h"

/*!\brief Extracts a data distribution from input data:
  * TypeSet or simply ptr + size
  * ArrayND (will try to work with the getData() pointer)
  * ValueSeries (will try to work with the arr() pointer)
*/

template <class vT>
mClass(Algo) DataDistributionExtracter : public ParallelTask
{ mODTextTranslationClass(DataDistributionExtracter)
public:

    typedef ArrayND<vT>			ArrNDType;
    typedef DataDistribution<vT>	DistribType;

			DataDistributionExtracter( const ArrNDType& arr )
			    : arrnd_(&arr)
			    , vs_(0)
			    , distrib_(new DistribType)
			    , nrbins_(mUdf(int))
			    , bounds_(mUdf(vT),mUdf(vT))
			    , totalsz_(arr.info().getTotalSz())
			    , arr_(arr.getData())	{}
			DataDistributionExtracter( const vT* arr, od_int64 sz )
			    : arr_(arr)
			    , vs_(0)
			    , distrib_(new DistribType)
			    , nrbins_(mUdf(int))
			    , bounds_(mUdf(vT),mUdf(vT))
			    , totalsz_(sz)
			    , arrnd_(0)			{}
			DataDistributionExtracter( const ValueSeries<vT>& vs,
						   od_int64 sz )
			    : vs_(&vs)
			    , arr_(vs.arr())
			    , distrib_(new DistribType)
			    , nrbins_(mUdf(int))
			    , bounds_(mUdf(vT),mUdf(vT))
			    , totalsz_(sz)
			    , arrnd_(0)			{}
			DataDistributionExtracter( const TypeSet<vT>& ts )
			    : DataDistributionExtracter(ts.arr(),ts.size())
							{}
    virtual		~DataDistributionExtracter()	{}

    void		setNrBins( int nr )		{ nrbins_ = nr; }
    void		setBounds( Interval<vT> intv )	{ bounds_ = intv; }

    virtual od_int64	nrIterations() const		{ return totalsz_; }

    RefMan<DistribType>	getDistribution()		{ return distrib_; }

protected:

    const ArrayND<vT>*		arrnd_;
    const vT*			arr_;
    const ValueSeries<vT>*	vs_;
    const od_int64		totalsz_;
    RefMan<DistribType>		distrib_;
    int				nrbins_;
    Interval<vT>		bounds_;

    virtual bool		doPrepare(int);
    virtual bool		doWork(od_int64,od_int64,int);

    void			determineBounds();
    void			includeInBounds(vT,const bool,const bool);
    void			putInBin(vT,TypeSet<vT>&,
					const SamplingData<vT>&,const int);

};


template <class vT> inline
bool DataDistributionExtracter<vT>::doPrepare( int )
{
    if ( totalsz_ < 1 )
	return false;
    if ( !arrnd_ && !arr_ && !vs_ )
	{ pErrMsg("Duh"); return false; }

    if ( mIsUdf(nrbins_) )
    {
	nrbins_ = (int)(totalsz_ / 64);
	if ( nrbins_ < 8 )
	    nrbins_ = 8;
	if ( nrbins_ > 1024 )
	    nrbins_ = 1024;
    }

    if ( mIsUdf(bounds_.start) || mIsUdf(bounds_.stop) )
    {
	determineBounds();
	if ( mIsUdf(bounds_.start) || mIsUdf(bounds_.stop) )
	    { bounds_.start = vT(0); bounds_.stop = vT(1); }
    }

    if ( bounds_.start == bounds_.stop )
	bounds_.stop = bounds_.start + (vT)1;
    else if ( bounds_.start > bounds_.stop )
	std::swap( bounds_.start, bounds_.stop );

    SamplingData<vT> sd;
    sd.step = (bounds_.stop - bounds_.start) / nrbins_;
    sd.start = bounds_.start + sd.step*0.5f;
    distrib_ = new DistribType( sd, nrbins_ );

    return true;
}


template <class vT> inline
void DataDistributionExtracter<vT>::includeInBounds( vT val,
			    const bool needmin, const bool needmax )
{
    if ( mIsUdf(val) )
	return;
    if ( needmin && (mIsUdf(bounds_.start) || val < bounds_.start) )
	bounds_.start = val;
    if ( needmax && (mIsUdf(bounds_.stop) || val > bounds_.stop) )
	bounds_.stop = val;
}


template <class vT> inline
void DataDistributionExtracter<vT>::determineBounds()
{
    const bool needmin = mIsUdf(bounds_.start);
    const bool needmax = mIsUdf(bounds_.stop);
    if ( arr_ )
    {
	for ( const vT* cur = arr_; cur != arr_ + totalsz_; cur++ )
	    includeInBounds( *cur, needmin, needmax);
    }
    else if ( vs_ )
    {
	for ( int idx=0; idx<totalsz_; idx++ )
	    includeInBounds( (*vs_)[idx], needmin, needmax );
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	while ( iter.next() )
	    includeInBounds( arrnd_->getND( iter.getPos() ), needmin, needmax );
    }
}


template <class vT> inline
void DataDistributionExtracter<vT>::putInBin( vT val, TypeSet<vT>& subdistrib,
		    const SamplingData<vT>& sd, const int nrbins )
{
    if ( !mIsUdf(val) )
    {
	const int ibin
		= DataDistribution<vT>::getBinNrFor( val, sd, nrbins );
	subdistrib[ibin]++;
    }
}


template <class vT> inline
bool DataDistributionExtracter<vT>::doWork( od_int64 start, od_int64 stop, int )
{
    TypeSet<vT> subdistrib( distrib_->size(), 0 );
    const SamplingData<vT> sd = distrib_->sampling();
    const int nrbins = distrib_->size();
    if ( arr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    putInBin( arr_[idx], subdistrib, sd, nrbins );
    }
    else if ( vs_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    putInBin( (*vs_)[idx], subdistrib, sd, nrbins );
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	iter.setGlobalPos( start-1 );
	while ( iter.next() )
	    putInBin( arrnd_->getND(iter.getPos()), subdistrib, sd, nrbins );
    }

    distrib_->add( subdistrib.arr() );
    return true;
}
