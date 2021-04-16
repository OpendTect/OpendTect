#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		January 2017
________________________________________________________________________

-*/

#include "algomod.h"
#include "typeset.h"
#include "samplingdata.h"
#include "sharedobject.h"

template <class VT> class DataDistributionChanger;
template <class VT> class DataDistributionInfoExtracter;
template <class VT> class DataDistributionIter;


/*!\brief Sharable data distribution. Sampling defaults to 0 step 1.

  The data in this object can be either true 'frequencies' or 'histogram'
  values. More generally, if you want to obtain a 'true' distribution in the
  mathematical sense of the word, then you should use 'normalise', which
  assures that the sum of the values is 1.

  The SamplingData describes the positions of the bin *centers*. Thus, if
  you have a hard start and stop, calculate the step like (stop-start)/nrbins,
  then put sampling.start at start + 0.5*step.

  Note that it is a bad idea to have negative data (P<0). We're not checking.

  Also note that pos_type and value_type are the same typedefs. The position
  in a distribution is a data value.

*/


template <class VT>
mClass(Algo) DataDistribution : public SharedObject
{
public:

    typedef VT			value_type;
    typedef value_type		pos_type;
    typedef TypeSet<VT>		SetType;
    mUseTemplType( SetType,		size_type );
    mUseTemplType( SetType,		idx_type );
    typedef SamplingData<pos_type> SamplingType;
    typedef Interval<VT>	RangeType;

    inline			DataDistribution()
				    : sampling_(pos_type(0),pos_type(1)) {}
    inline			DataDistribution(size_type);
    inline			DataDistribution(SamplingType,size_type n=256);
    inline			DataDistribution(const SetType&);
    inline			DataDistribution(const SetType&,SamplingType);
				mDeclMonitorableAssignment(DataDistribution);

    inline size_type		size() const;
    inline bool			isEmpty() const		{ return size() < 1; }
    inline void			setEmpty();
    mImplSimpleMonitoredGetSet( inline,sampling,setSampling,SamplingType,
				sampling_,cSamplingChange());
    void			setSize(size_type);

    inline VT			get(idx_type,bool cumulative=false) const;
    inline VT			operator[](idx_type) const;
    inline pos_type		binPos(idx_type) const;
    inline VT			valueAt(pos_type,bool cumulative) const;
    inline idx_type		getBinNr(pos_type) const;
    inline SetType		getSet( bool cum ) const
				{ mLock4Read(); return cum ? cumdata_ : data_; }

    inline void			set(idx_type,VT);    //!< slow, O(N)
    inline void			set(const VT*);	    //!< fast, no checks
    inline void			add(const VT*);	    //!< fast, no checks

    inline bool			isNormalised() const
				{ return sumOfValues() == VT(1); }
    inline VT			sumOfValues() const;
    inline VT			maxValue() const;
    inline RangeType		dataRange() const;

    inline pos_type		positionForCumulative(VT) const;
    inline pos_type		medianPosition() const;
    inline void			getAvgStd(pos_type& avg,pos_type& std) const;
    inline VT*			getArr( bool cum ) const
				{ return cum ? cumdata_.arr() : data_.arr(); }
				//!< for fast non-shared usage

    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cSamplingChange()	{ return 3; }

    static const DataDistribution<VT>& getEmptyDistrib();
    static inline idx_type	getBinNrFor(pos_type,const SamplingType&,
					    size_type nrbins);

protected:

    inline		~DataDistribution();

    SetType		data_;
    SetType		cumdata_;
    SamplingType	sampling_;

    void		setCumData(int);
    inline VT		gtMax(int* idxat=0) const;

    friend class	DataDistributionChanger<VT>;
    friend class	DataDistributionInfoExtracter<VT>;
    friend class	DataDistributionIter<VT>;

};

typedef DataDistribution<float> FloatDistrib;


template <class VT> inline
DataDistribution<VT>::DataDistribution( const DataDistribution<VT>& oth )
    : SharedObject(oth)
    , data_(oth.data_)
    , cumdata_(oth.cumdata_)
    , sampling_(oth.sampling_)
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( size_type nrbins )
    : data_(nrbins,0)
    , cumdata_(nrbins,0)
    , sampling_(pos_type(0),pos_type(1))
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( SamplingType sd, size_type nrbins )
    : data_(nrbins,0)
    , cumdata_(nrbins,0)
    , sampling_(sd)
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( const SetType& d )
    : data_(d.size(),0)
    , cumdata_(d.size(),0)
    , sampling_(pos_type(0),pos_type(1))
{
    set( d.arr() );
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( const SetType& d, SamplingType sd )
    : data_(d.size(),0)
    , cumdata_(d.size(),0)
    , sampling_(sd)
{
    set( d.arr() );
}


template <class VT> inline
DataDistribution<VT>::~DataDistribution()
{
    sendDelNotif();
}


mGenImplMonitorableAssignment(template <class VT> inline,DataDistribution<VT>,
			      SharedObject);


template <class VT> inline
void DataDistribution<VT>::copyClassData( const DataDistribution<VT>& oth )
{
    data_ = oth.data_;
    cumdata_ = oth.cumdata_;
    sampling_ = oth.sampling_;
}


template <class VT> inline
Monitorable::ChangeType DataDistribution<VT>::compareClassData(
				const DataDistribution<VT>& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( data_, cDataChange() );
    mHandleMonitorableCompare( sampling_, cSamplingChange() );
    mDeliverMonitorableCompare();
}


template <class VT> inline
typename DataDistribution<VT>::size_type DataDistribution<VT>::size() const
{
    mLock4Read();
    return data_.size();
}


template <class VT> inline
typename DataDistribution<VT>::idx_type DataDistribution<VT>::getBinNrFor(
			pos_type pos, const SamplingType& sd, size_type nbins )
{
    const float fbin = sd.getfIndex( pos );
    idx_type ret;
    if ( fbin < 0 )
	ret = 0;
    else
    {
	ret = (idx_type)( fbin + 0.5f );
	if ( ret >= nbins )
	    ret = nbins - 1;
    }
    return ret;
}


template <class VT> inline
typename DataDistribution<VT>::idx_type DataDistribution<VT>::getBinNr(
						pos_type p ) const
{
    mLock4Read();
    return getBinNrFor( p, sampling_, data_.size() );
}


template <class VT> inline
VT DataDistribution<VT>::operator[]( idx_type idx ) const
{
    return get( idx, false );
}


template <class VT> inline
VT DataDistribution<VT>::get( idx_type idx, bool cumulative ) const
{
    mLock4Read();
    return cumulative ? cumdata_[idx] : data_[idx];
}


template <class VT> inline typename DataDistribution<VT>::pos_type
DataDistribution<VT>::binPos( idx_type idx ) const
{
    mLock4Read();
    return sampling_.atIndex( idx );
}


template <class VT> inline
VT DataDistribution<VT>::valueAt( pos_type pos, bool cumulative ) const
{
    mLock4Read();
    const SetType& vals = cumulative ? cumdata_ : data_;
    const int sz = vals.size();
    if ( sz < 1 )
	return mUdf(VT);

    pos_type fidx = sampling_.getfIndex( pos );
    if ( fidx < -0.5f )
	fidx = -0.5f;
    else if ( fidx > vals.size()-0.5f )
	fidx = vals.size()-0.5f;
    if ( cumulative )
	fidx -= 0.5f;

    const int aftidx = getLimited( ((int)fidx + 1), 0, sz );
    const int beforeidx = aftidx - 1;

    const VT valbefore = beforeidx<0 ? (cumulative?VT(0):vals[0])
				     : vals[ beforeidx ];
    const VT valafter = aftidx>sz-2 ? vals.last() : vals[ aftidx ] ;
    const pos_type relpos = (fidx - beforeidx);
    return valbefore + (valafter-valbefore) * relpos;
}


template <class VT> inline
void DataDistribution<VT>::setEmpty()
{
    mLock4Read();
    if ( data_.isEmpty() )
	return;
    if ( !mLock2Write() && data_.isEmpty() )
	return;

    data_.setEmpty();
    mSendEntireObjChgNotif();
}


template <class VT> inline
void DataDistribution<VT>::set( idx_type isamp, VT val )
{
    mLock4Read();
    if ( !data_.validIdx(isamp) || data_[isamp] == val )
	return;
    if ( !mLock2Write() && (isamp>=data_.size() || data_[isamp] == val) )
	return;

    const VT diff = val - data_[isamp];
    data_[isamp] = val;
    const size_type sz = data_.size();
    for ( idx_type idx=isamp+1; idx<sz; idx++ )
	data_[idx] += diff;
    mSendChgNotif( cDataChange(), isamp );
}


template <class VT> inline
void DataDistribution<VT>::add( const VT* vals )
{
    mLock4Write();
    const size_type sz = data_.size();
    VT add2cumulative = 0;
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	add2cumulative += vals[idx];
	data_[idx] += vals[idx];
	cumdata_[idx] += add2cumulative;
    }

    mSendChgNotif( cDataChange(), cUnspecChgID() );
}


template <class VT> inline
void DataDistribution<VT>::setCumData( int idx )
{
    cumdata_[idx] = data_[idx] + (idx<1 ? VT(0) : cumdata_[idx-1]);
}


template <class VT> inline
void DataDistribution<VT>::set( const VT* vals )
{
    mLock4Write();
    const size_type sz = data_.size();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	data_[idx] = vals[idx];
	setCumData( idx );
    }

    mSendChgNotif( cDataChange(), cUnspecChgID() );
}


template <class VT> inline
VT DataDistribution<VT>::sumOfValues() const
{
    mLock4Read();
    return cumdata_.last();
}


template <class VT> inline
VT DataDistribution<VT>::maxValue() const
{
    mLock4Read();
    return gtMax();
}


template <class VT> inline
typename DataDistribution<VT>::RangeType DataDistribution<VT>::dataRange() const
{
    mLock4Read();
    const pos_type hstep = sampling_.step * pos_type(0.5);
    return RangeType( sampling_.start - hstep,
		      sampling_.atIndex(data_.size()-1) + hstep );
}


template <class VT> inline
VT DataDistribution<VT>::gtMax( int* idxat ) const
{
    const size_type sz = data_.size();
    if ( idxat )
	*idxat = 0;

    if ( sz < 2 )
	return sz == 1 ? data_[0] : VT(0);

    VT ret = data_[0];
    for ( idx_type idx=1; idx<sz; idx++ )
    {
	const VT val = data_[idx];
	if ( val > ret )
	{
	    ret = val;
	    if ( idxat )
		*idxat = idx;
	}
    }

    return ret;
}


template <class VT> inline typename DataDistribution<VT>::pos_type
DataDistribution<VT>::positionForCumulative( VT val ) const
{
    if ( mIsUdf(val) )
	return VT(1);

    mLock4Read();
    const size_type sz = cumdata_.size();
    if ( sz < 2 )
	return sz == 1 ? sampling_.start : mUdf(pos_type);
    else if ( val <= VT(0) )
	return sampling_.start - pos_type(0.5) * sampling_.step;
    else if ( val >= cumdata_.last() )
	return sampling_.start + pos_type(sz-0.5) * sampling_.step;

    // this function is mostly used for clipping which is at start or end
    // simple bisection may then not be optimal.
    // if not, then this will still be 2 times faster (on average)

    if ( val < cumdata_[sz/2] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const VT nextval = cumdata_[idx];
	    if ( val <= nextval )
	    {
		pos_type prevpos = sampling_.start + pos_type(idx-0.5)
						    * sampling_.step;
		if ( val == nextval )
		    return prevpos + sampling_.step;

		const VT prevval = idx ? cumdata_[idx-1] : VT(0);
		const pos_type relpos = (val - prevval) / (nextval - prevval);
		return prevpos + sampling_.step * relpos;
	    }
	}
    }
    else
    {
	for ( int idx=sz-1; idx>0; idx-- )
	{
	    const VT prevval = cumdata_[idx-1];
	    if ( val >= prevval )
	    {
		pos_type prevpos = sampling_.start + VT(idx-0.5)
						    * sampling_.step;
		if ( prevval == val )
		    return prevpos;

		const VT nextval = cumdata_[idx];
		const pos_type relpos = (val - prevval) / (nextval - prevval);
		return prevpos + sampling_.step * relpos;
	    }
	}
    }

    // should not reach
    return VT(0);
}


template <class VT> inline typename DataDistribution<VT>::pos_type
DataDistribution<VT>::medianPosition() const
{
    mLock4Read();
    if ( cumdata_.size() < 2 )
	return sampling_.start;

    return positionForCumulative( cumdata_.last() * VT(0.5) );
}


template <class VT> inline void DataDistribution<VT>::getAvgStd( pos_type& avg,
							 pos_type& std ) const
{
    mLock4Read();
    const auto sz = cumdata_.size();
    if ( sz < 2 || cumdata_[sz-1] == VT(0) )
	{ avg = sampling_.start; std = mUdf(pos_type); return; }

    pos_type wtsum = VT(0);
    for ( auto idx=0; idx<sz; idx++ )
	wtsum += data_[idx] * sampling_.atIndex( idx );
    avg = wtsum / cumdata_[sz-1];

    pos_type sumsqdiffs = VT(0);
    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto diff = sampling_.atIndex(idx) - avg;
	sumsqdiffs += data_[idx] * diff * diff;
    }
    std = Math::Sqrt( sumsqdiffs / cumdata_[sz-1] );
}


template <class VT> inline
const DataDistribution<VT>& DataDistribution<VT>::getEmptyDistrib()
{
    mDefineStaticLocalObject( RefMan< DataDistribution<VT> >, theempty,
					= new DataDistribution<VT> );
    return *theempty;
}
