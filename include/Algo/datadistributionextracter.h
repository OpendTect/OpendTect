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
#include "paralleltask.h"

/*!\brief Extracts a data distribution from an ArrayND */

template <class vT>
mClass(Algo) DataDistributionExtracter : public ParallelTask
{ mODTextTranslationClass(DataDistributionExtracter)
public:

    typedef ArrayND<vT>			ArrNDType;
    typedef DataDistribution<vT>	DistribType;

			DataDistributionExtracter( const ArrNDType& arr )
			    : arrnd_(&arr)
			    , nrbins_(mUdf(int))
			    , bounds_(mUdf(vT),mUdf(vT))
			    , totalsz_(arr.info().getTotalSz())
			    , arr_(arr.getData())	{}
			DataDistributionExtracter( const vT* arr, od_int64 sz )
			    : arr_(arr)
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
    const od_int64		totalsz_;
    const vT*			arr_;
    RefMan<DistribType>		distrib_;
    int				nrbins_;
    Interval<vT>		bounds_;

    virtual bool		doPrepare(int);
    virtual bool		doWork(od_int64,od_int64,int);

    void			determineBounds();

};


template <class vT>
bool DataDistributionExtracter<vT>::doPrepare( int )
{
    if ( totalsz_ < 1 )
	return false;
    if ( !arrnd_ && !arr_ )
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
	determineBounds();

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


template <class vT>
void DataDistributionExtracter<vT>::determineBounds()
{
    const bool needmin = mIsUdf(bounds_.start);
    const bool needmax = mIsUdf(bounds_.stop);
    if ( arr_ )
    {
	if ( needmin ) bounds_.start = *arr_;
	if ( needmax ) bounds_.stop = *arr_;
	for ( const vT* cur = arr_; cur != arr_ + totalsz_; cur++ )
	{
	    if ( mIsUdf(*cur) )
		continue;
	    if ( needmin && (mIsUdf(bounds_.start) || *cur < bounds_.start) )
		bounds_.start = *cur;
	    if ( needmax && (mIsUdf(bounds_.stop) || *cur > bounds_.stop) )
		bounds_.stop = *cur;
	}
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	while ( iter.next() )
	{
	    const vT val = arrnd_->getND( iter.getPos() );
	    if ( mIsUdf(val) )
		continue;
	    if ( needmin && (mIsUdf(bounds_.start) || val < bounds_.start) )
		bounds_.start = val;
	    if ( needmax && (mIsUdf(bounds_.stop) || val > bounds_.stop) )
		bounds_.stop = val;
	}
    }
}


template <class vT>
bool DataDistributionExtracter<vT>::doWork( od_int64 start, od_int64 stop, int )
{
    TypeSet<vT> subdistrib( distrib_->size(), 0 );
    const SamplingData<vT> sd = distrib_->sampling();
    const int nrbins = distrib_->size();
    if ( arr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    float val = arr_[idx];
	    if ( !mIsUdf(val) )
	    {
		const int ibin
			= DataDistribution<vT>::getBinFor( val, sd, nrbins );
		subdistrib[ibin]++;
	    }
	}
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	iter.setGlobalPos( start-1 );
	while ( iter.next() )
	{
	    const vT val = arrnd_->getND( iter.getPos() );
	    if ( !mIsUdf(val) )
	    {
		const int ibin
			= DataDistribution<vT>::getBinFor( val, sd, nrbins );
		subdistrib[ibin]++;
	    }
	}
    }

    distrib_->add( subdistrib.arr() );
    return true;
}
