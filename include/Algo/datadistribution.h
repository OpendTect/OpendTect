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
#include "monitoriter.h"

template <class VT> class DataDistributionIter;


/*!\brief Sharable data distribution. Sampling defaults to 0 step 1.

  The data in this object can be either true 'frequencies' or 'histogram'
  values. More generally, if you want to obtain a 'true' distribution in the
  mathematical sense of the word, then you should use 'normalise', which
  assures that the sum of he values is 1.

  The SamplingData describes the positions of the bin *centers*. Thus, if
  you have a hard start and stop, calculate the step like (stop-start)/nrbins,
  then put sampling.start at start + 0.5*step.

  Note that it is a bad idea to have negative data (P<0). We're not checking.

*/

template <class VT>
mClass(Algo) DataDistribution : public SharedObject
{
public:

    typedef float		PosType;
    typedef VT			ValueType;
    typedef TypeSet<VT>		SetType;
    typedef typename SetType::size_type	size_type;
    typedef size_type		IdxType;
    typedef SamplingData<PosType> SamplingType;

    inline			DataDistribution()
				    : sampling_(PosType(0),PosType(1)) {}
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

    inline VT			get(IdxType,bool cumulative=false) const;
    inline VT			operator[](IdxType) const;
    inline VT			valueAt(PosType,bool cumulative) const;
    inline IdxType		getBinNr(PosType) const;
    inline SetType		getSet( bool cum ) const
				{ mLock4Read(); return cum ? cumdata_ : data_; }

    inline void			set(IdxType,VT);    //!< slow, O(N)
    inline void			set(const VT*);	    //!< fast, no checks
    inline void			add(const VT*);	    //!< fast, no checks

    inline bool			isNormalised() const
				{ return sumOfValues() == VT(1); }
    inline VT			sumOfValues() const;
    inline VT			maxValue() const;
    inline void			normalise(bool in_the_math_sense=true);
				//!< if !math, sets max to 1
    inline bool			deSpike(VT ratioaboverg=VT(0.5));
				//!< Returns whether any change made
				//!< Note that your distrib is no longer correct
				//!< Useful for nice displays

    inline void			getCurve(SetType& xvals,SetType& yvals,
				        bool limitspikes=false) const;
    inline void			getRanges(Interval<PosType>& xrg,
					  Interval<PosType>& yrg) const;
    inline PosType		positionForCumulative(VT) const;
    inline VT*			getArr( bool cum ) const
				{ return cum ? cumdata_.arr() : data_.arr(); }
				//!< for fast non-shared usage

    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cSamplingChange()	{ return 3; }

    static const DataDistribution<VT>& getEmptyDistrib();
    static inline IdxType	getBinNrFor(PosType,const SamplingType&,
					    size_type nrbins);

protected:

    inline		~DataDistribution();

    SetType		data_;
    SetType		cumdata_;
    SamplingType	sampling_;

    inline VT		gtMax() const;

    friend class	DataDistributionIter<VT>;

};


template <class VT>
mClass(Algo) DataDistributionIter
	: public MonitorableIter4Read< typename DataDistribution<VT>::IdxType >
{
public:

    typedef MonitorableIterBase< typename DataDistribution<VT>::IdxType >
						base_type;
    typedef DataDistribution<VT>		DistribType;
    typedef typename DistribType::PosType	PosType;
    typedef typename DistribType::IdxType	IdxType;

    inline	    DataDistributionIter( const DistribType& d )
			: MonitorableIter4Read<IdxType>(d,0,d.size()-1) {}
    inline	    DataDistributionIter( const DataDistributionIter& oth )
			: MonitorableIter4Read<IdxType>(oth)		{}
    inline const DistribType& distrib() const
		    { return static_cast<const DistribType&>(
					    base_type::monitored() ); }

    inline bool	    isValid() const	{ return base_type::isValid(); }
    inline VT	    value() const
		    { return isValid() ? distrib().data_[base_type::curidx_]
				       : mUdf(VT); }
    inline VT	    cumValue() const
		    { return isValid() ? distrib().cumdata_[base_type::curidx_]
				       : mUdf(VT); }
    inline PosType  position() const
		    { return isValid()
			    ? distrib().sampling().atIndex(base_type::curidx_)
			    : mUdf(PosType); }

    mDefNoAssignmentOper(DataDistributionIter)

};



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
    , sampling_(PosType(0),PosType(1))
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
    , sampling_(PosType(0),PosType(1))
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
typename DataDistribution<VT>::IdxType DataDistribution<VT>::getBinNrFor(
			PosType pos, const SamplingType& sd, size_type nbins )
{
    const float fbin = sd.getfIndex( pos );
    IdxType ret;
    if ( fbin < 0 )
	ret = 0;
    else
    {
	ret = (IdxType)( fbin + 0.5f );
	if ( ret >= nbins )
	    ret = nbins - 1;
    }
    return ret;
}


template <class VT> inline
typename DataDistribution<VT>::IdxType DataDistribution<VT>::getBinNr(
						PosType p ) const
{
    mLock4Read();
    return getBinNrFor( p, sampling_, data_.size() );
}


template <class VT> inline
VT DataDistribution<VT>::operator[]( IdxType idx ) const
{
    return get( idx, false );
}


template <class VT> inline
VT DataDistribution<VT>::get( IdxType idx, bool cumulative ) const
{
    mLock4Read();
    return cumulative ? cumdata_[idx] : data_[idx];
}

template <class VT> inline
VT DataDistribution<VT>::valueAt( PosType pos, bool cumulative ) const
{
    mLock4Read();
    const SetType& vals = cumulative ? cumdata_ : data_;
    const int sz = vals.size();
    if ( sz < 1 )
	return mUdf(VT);

    PosType fidx = sampling_.getfIndex( pos );
    if ( fidx < -0.5f )
	fidx = -0.5f;
    else if ( fidx > vals.size()-0.5f )
	fidx = vals.size()-0.5f;
    if ( cumulative )
	fidx -= 0.5f;

    int aftidx = (int)fidx + 1;
    if ( aftidx >= sz )
	aftidx = sz;
    if ( aftidx < 0 )
	aftidx = 0;
    const int beforeidx = aftidx - 1;

    const VT valbefore = beforeidx<0 ? (cumulative?VT(0):vals[0])
				     : vals[ beforeidx ];
    const VT valafter = vals[ aftidx ];
    const PosType relpos = (fidx - beforeidx);
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
void DataDistribution<VT>::set( IdxType isamp, VT val )
{
    mLock4Read();
    if ( !data_.validIdx(isamp) || data_[isamp] == val )
	return;
    if ( !mLock2Write() && (isamp>=data_.size() || data_[isamp] == val) )
	return;

    const VT diff = val - data_[isamp];
    data_[isamp] = val;
    const size_type sz = data_.size();
    for ( IdxType idx=isamp+1; idx<sz; idx++ )
	data_[idx] += diff;
    mSendChgNotif( cDataChange(), isamp );
}


template <class VT> inline
void DataDistribution<VT>::add( const VT* vals )
{
    mLock4Write();
    const size_type sz = data_.size();
    VT add2cumulative = 0;
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	add2cumulative += vals[idx];
	data_[idx] += vals[idx];
	cumdata_[idx] += add2cumulative;
    }

    mSendChgNotif( cDataChange(), cUnspecChgID() );
}


template <class VT> inline
void DataDistribution<VT>::set( const VT* vals )
{
    mLock4Write();
    const size_type sz = data_.size();
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	data_[idx] = vals[idx];
	if ( idx == 0 )
	    cumdata_[idx] = vals[idx];
	else
	    cumdata_[idx] = cumdata_[idx-1] + vals[idx];
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
VT DataDistribution<VT>::gtMax() const
{
    size_type sz = data_.size();
    if ( sz < 2 )
	return sz == 1 ? data_[0] : VT(0);

    VT ret = data_[0];
    for ( IdxType idx=1; idx<sz; idx++ )
    {
	const VT val = data_[idx];
	if ( val > ret )
	    ret = val;
    }

    return ret;
}


template <class VT> inline
void DataDistribution<VT>::normalise( bool in_the_math_sense )
{
    mLock4Write();
    size_type sz = data_.size();
    if ( sz < 1 )
	return;

    const VT divby = in_the_math_sense ? cumdata_[sz-1] : gtMax();
    if ( divby == VT(0) || divby == VT(1) )
	return;

    for ( IdxType idx=0; idx<sz; idx++ )
    {
	data_[idx] /= divby;
	if ( idx == 0 )
	    cumdata_[idx] = data_[idx];
	else
	    cumdata_[idx] = cumdata_[idx-1] + data_[idx];
    }

    // last should now be pretty close to 1, but let's make it exact
    cumdata_[sz-1] = VT(1);

    mSendEntireObjChgNotif();
}


template <class VT> inline bool DataDistribution<VT>::deSpike( VT cutoff )
{
    mLock4Write();
    const int sz = data_.size();
    if ( sz < 6 )
	return false;

    VT maxval = data_[0];
    VT runnerupval = maxval;
    VT minval = maxval;
    IdxType idxatmax = 0;

    for ( int idx=1; idx<sz; idx++ )
    {
	const VT val = data_[idx];
	if ( val < minval )
	    minval = val;
	else if ( val > maxval )
	{
	    runnerupval = maxval;
	    maxval = val;
	    idxatmax = idx;
	}
    }

    const VT unspikedvalrg = runnerupval - minval;
    const VT spikelimit = minval + VT(1+cutoff) * unspikedvalrg;
    if ( maxval > spikelimit )
    {
	data_[idxatmax] = spikelimit;
	return true;
    }
    return false;
}


template <class VT> inline
void DataDistribution<VT>::getCurve( SetType& xvals, SetType& yvals,
				     bool limitspikes ) const
{
    xvals.setEmpty(); yvals.setEmpty();

    if ( limitspikes )
    {
	RefMan< DataDistribution<VT> > despiked = clone();
	despiked->deSpike();
	despiked->getCurve( xvals, yvals, false );
    }
    else
    {
	DataDistributionIter<VT> iter( *this );
	while ( iter.next() )
	{
	    xvals += iter.position();
	    yvals += iter.value();
	}
    }
}


template <class VT> inline
void DataDistribution<VT>::getRanges( Interval<PosType>& xrg,
				      Interval<PosType>& yrg ) const
{
    mLock4Read();
    xrg.start = sampling_.start;
    if ( data_.size() < 1 )
    {
	xrg.stop = xrg.start;
	yrg.start = yrg.stop = 0;
    }
    else
    {
	xrg.stop = sampling_.atIndex( data_.size()-1 );
	yrg.start = yrg.stop = data_[0];
	for ( IdxType idx=1; idx<data_.size(); idx++ )
	{
	    const VT val = data_[idx];
	    if ( val > yrg.stop )
		yrg.stop = val;
	    else if ( val < yrg.start )
		yrg.start = val;
	}
    }
}


template <class VT> inline typename DataDistribution<VT>::PosType
DataDistribution<VT>::positionForCumulative( VT val ) const
{
    if ( mIsUdf(val) )
	return VT(1);

    mLock4Read();
    const size_type sz = cumdata_.size();
    if ( sz < 2 )
	return sz == 1 ? sampling_.start : mUdf(PosType);
    else if ( val <= VT(0) )
	return sampling_.start - PosType(0.5) * sampling_.step;
    else if ( val >= cumdata_.last() )
	return sampling_.start + PosType(sz-0.5) * sampling_.step;

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
		PosType prevpos = sampling_.start + PosType(idx-0.5)
						    * sampling_.step;
		if ( val == nextval )
		    return prevpos + sampling_.step;

		const VT prevval = idx ? cumdata_[idx-1] : VT(0);
		const PosType relpos = (val - prevval) / (nextval - prevval);
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
		PosType prevpos = sampling_.start + VT(idx-0.5)
						    * sampling_.step;
		if ( prevval == val )
		    return prevpos;

		const VT nextval = cumdata_[idx];
		const PosType relpos = (val - prevval) / (nextval - prevval);
		return prevpos + sampling_.step * relpos;
	    }
	}
    }

    // should not reach
    return VT(0);
}


template <class VT> inline
const DataDistribution<VT>& DataDistribution<VT>::getEmptyDistrib()
{
    mDefineStaticLocalObject( RefMan< DataDistribution<VT> >, theempty,
					= new DataDistribution<VT> );
    return *theempty;
}
