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

  Note that it is a bad idea to put negative values in the data. We're
  not checking.

*/

template <class VT>
mClass(Algo) DataDistribution : public SharedObject
{
public:

    typedef VT			ValueType;
    typedef TypeSet<VT>		SetType;
    typedef typename SetType::size_type	size_type;
    typedef size_type		IdxType;
    typedef SamplingData<VT>	SamplingType;

    inline			DataDistribution() : sampling_(VT(0),VT(1)) {}
    inline			DataDistribution(size_type);
    inline			DataDistribution(SamplingType,size_type n=256);
    inline			DataDistribution(const SetType&);
    inline			DataDistribution(const SetType&,SamplingType);
				mDeclMonitorableAssignment(DataDistribution);

    inline operator		SetType() const		{ return data(); }

    inline size_type		size() const;
    inline bool			isEmpty() const		{ return size() < 1; }
    inline void			setEmpty();
    mImplSimpleMonitoredGetSet( inline,data,setData,SetType,data_,
				cDataChange());
    mImplSimpleMonitoredGetSet( inline,sampling,setSampling,SamplingType,
				sampling_,cSamplingChange());

    inline VT			operator[](IdxType idx) const;
    inline void			set(IdxType,VT);
    VT*				getArr()		{ return data_.arr(); }
				//!< for fast non-shared usage

    inline VT			sumOfValues() const;
    inline void			normalise();
    inline void			getCurve(TypeSet<VT>& xvals,TypeSet<VT>& yvals,
				        bool limitspikes=false) const;
    inline void			getRanges(Interval<VT>& xrg,
					  Interval<VT>& yrg) const;

    static ChangeType		cDataChange()		{ return 2; }
    static ChangeType		cSamplingChange()	{ return 3; }

    static const DataDistribution<VT>& getEmptyDistrib();

protected:

    inline		~DataDistribution();

    SetType		data_;
    SamplingType	sampling_;

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
    inline VT	    position() const
		    { return isValid()
			    ? distrib().sampling().atIndex(base_type::curidx_)
			    : mUdf(VT); }

    mDefNoAssignmentOper(DataDistributionIter)

};



template <class VT> inline
DataDistribution<VT>::DataDistribution( const DataDistribution<VT>& oth )
    : SharedObject(oth)
    , data_(oth.data_)
    , sampling_(oth.sampling_)
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( size_type nrbins )
    : data_(nrbins,0)
    , sampling_(VT(0),VT(1))
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( SamplingType sd, size_type nrbins )
    : data_(nrbins,0)
    , sampling_(sd)
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( const SetType& d )
    : data_(d)
    , sampling_(VT(0),VT(1))
{
}


template <class VT> inline
DataDistribution<VT>::DataDistribution( const SetType& d, SamplingType sd )
    : data_(d)
    , sampling_(sd)
{
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
VT DataDistribution<VT>::operator[]( IdxType idx ) const
{
    mLock4Read();
    return data_[idx];
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
void DataDistribution<VT>::set( IdxType idx, VT val )
{
    mLock4Read();
    if ( !data_.validIdx(idx) || data_[idx] == val )
	return;
    if ( !mLock2Write() && (idx>=data_.size() || data_[idx] == val) )
	return;

    data_[idx] = val;
    mSendChgNotif( cDataChange(), idx );
}


template <class VT> inline
VT DataDistribution<VT>::sumOfValues() const
{
    mLock4Read();
    const size_type sz = size();
    VT sumvals = VT(0);
    for ( IdxType idx=0; idx<sz; idx++ )
	sumvals += data_[idx];
    return sumvals;
}


template <class VT> inline
void DataDistribution<VT>::normalise()
{
    const VT sumvals = sumOfValues();
    if ( sumvals == VT(0) )
	return;

    mLock4Write();
    const size_type sz = size();
    for ( IdxType idx=0; idx<sz; idx++ )
	data_[idx] /= sumvals;

    mSendEntireObjChgNotif();
}


template <class VT> inline
void DataDistribution<VT>::getCurve( TypeSet<VT>& xvals, TypeSet<VT>& yvals,
				    bool limitspikes ) const
{
    xvals.setEmpty(); yvals.setEmpty();
    mLock4Read();
    if ( data_.isEmpty() )
	return;

    VT maxval = data_[0];
    VT runnerupval = maxval;
    VT minval = maxval;
    IdxType idxatmax = 0;

    DataDistributionIter<VT> iter( *this );
    while ( iter.next() )
    {
	xvals += iter.position();
	const VT val = iter.value();
	yvals += val;
	if ( val < minval )
	    minval = val;
	else if ( val > maxval )
	{
	    runnerupval = maxval;
	    maxval = val;
	    idxatmax = iter.curIdx();
	}
    }

    if ( limitspikes && xvals.size() > 5 )
    {
	const VT valrg = runnerupval - minval;
	const VT max4disp = minval + 1.5f * valrg;
	if ( maxval > max4disp )
	    yvals[idxatmax] = max4disp;
    }
}


template <class VT> inline
void DataDistribution<VT>::getRanges( Interval<VT>& xrg,
				      Interval<VT>& yrg ) const
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
	for ( int idx=1; idx<data_.size(); idx++ )
	{
	    const VT val = data_[idx];
	    if ( val > yrg.stop )
		yrg.stop = val;
	    else if ( val < yrg.start )
		yrg.start = val;
	}
    }
}


template <class VT> inline
const DataDistribution<VT>& DataDistribution<VT>::getEmptyDistrib()
{
    mDefineStaticLocalObject( DataDistribution<VT>, theempty, );
    return theempty;
}
