#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		January 2017
________________________________________________________________________

-*/

#include "datadistribution.h"
#include "monitoriter.h"
#include "iopar.h"
#include "keystrs.h"


template <class VT>
mClass(Algo) DataDistributionIter
	: public MonitorableIter4Read<typename DataDistribution<VT>::idx_type>
{
public:

    typedef MonitorableIterBase< typename DataDistribution<VT>::idx_type >
						base_type;
    typedef DataDistribution<VT>	DistribType;
    mUseTemplType( DistribType,	pos_type );
    mUseTemplType( DistribType,	idx_type );

    inline	    DataDistributionIter( const DistribType& d )
			: MonitorableIter4Read<idx_type>(d,0,d.size()-1) {}
    inline	    DataDistributionIter( const DataDistributionIter& oth )
			: MonitorableIter4Read<idx_type>(oth)		{}
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
    inline pos_type  position() const
		    { return isValid()
			    ? distrib().sampling().atIndex(base_type::curidx_)
			    : mUdf(pos_type); }

    mDefNoAssignmentOper(DataDistributionIter)

};


template <class VT>
mClass(Algo) DataDistributionInfoExtracter
{
public:

    typedef DataDistribution<VT>	DistribType;
    mUseTemplType( DistribType,	pos_type );
    mUseTemplType( DistribType,	idx_type );
    mUseTemplType( DistribType,	value_type );
    mUseTemplType( DistribType,	SetType );

    inline		DataDistributionInfoExtracter( const DistribType& d )
			    : distrib_(d)		{}

    inline bool		isRoughlySymmetrical(bool mustbearound0=false) const;
				//!< criterion: median value is near modus

    inline void		getCurve(SetType& xvals,SetType& yvals,
				 bool limitspikes=false) const;
    inline void		getRanges(Interval<pos_type>& xrg,
				  Interval<pos_type>& yrg) const;
    inline void		getAvgStdRMS(pos_type&,pos_type&,pos_type&) const;

    void		fillPar(IOPar&) const;

protected:

    const DistribType&	distrib_;

};


template <class VT>
mClass(Algo) DataDistributionChanger
{
public:

    typedef DataDistribution<VT>	DistribType;
    mUseTemplType( DistribType,	pos_type );
    mUseTemplType( DistribType,	idx_type );
    mUseTemplType( DistribType,	value_type );
    mUseTemplType( DistribType,	SetType );

    inline		DataDistributionChanger( DistribType& d )
			    : distrib_(d)		{}

    inline void		normalise(bool in_the_math_sense=true);
				//!< if !math, sets max to 1
    inline bool		deSpike(VT ratioaboverg=VT(0.4));
				//!< Returns whether any change made
				//!< Note that your distrib is no longer correct
				//!< Useful for nice displays

    void		usePar(const IOPar&);

protected:

    DistribType&	distrib_;

};



template <class VT> inline
void DataDistributionChanger<VT>::normalise( bool in_the_math_sense )
{
    mLockMonitorable4Write( distrib_ );
    const idx_type sz = distrib_.data_.size();
    if ( sz < 1 )
	return;

    const VT divby = in_the_math_sense ? distrib_.cumdata_[sz-1]
				       : distrib_.gtMax();
    if ( divby == VT(0) || divby == VT(1) )
	return;

    for ( idx_type idx=0; idx<sz; idx++ )
    {
	distrib_.data_[idx] /= divby;
	distrib_.setCumData( idx );
    }

    if ( in_the_math_sense )
	distrib_.cumdata_[sz-1] = VT(1); // just to avoid ugly roundoffs

    mSendMonitorableChgNotif( distrib_, DistribType::cDataChange(),
				Monitorable::cUnspecChgID() );
}


template <class VT> inline
bool DataDistributionChanger<VT>::deSpike( VT cutoff )
{
    mLockMonitorable4Write( distrib_ );
    const int sz = distrib_.data_.size();
    if ( sz < 6 )
	return false;

    VT maxval = distrib_.data_[0];
    VT runnerupval = maxval;
    VT minval = maxval;
    idx_type idxatmax = 0;

    for ( int idx=1; idx<sz; idx++ )
    {
	const VT val = distrib_.data_[idx];
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
	distrib_.data_[idxatmax] = spikelimit;
	mSendMonitorableChgNotif( distrib_, DistribType::cDataChange(),
				  Monitorable::cUnspecChgID() );
	return true;
    }
    return false;
}


template <class VT> inline
bool DataDistributionInfoExtracter<VT>::isRoughlySymmetrical(
					    bool onlyaround0 ) const
{
    mLockMonitorable4Read( distrib_ );
    idx_type maxidx; distrib_.gtMax( &maxidx );
    pos_type medpos = distrib_.medianPosition();
    pos_type diff = medpos - distrib_.sampling_.atIndex( maxidx );
    if ( diff < pos_type(0) )
	diff = -diff;
    if ( diff < distrib_.sampling_.step )
    {
	if ( !onlyaround0 )
	    return true;
	if ( medpos > -distrib_.sampling_.step
	  && medpos < distrib_.sampling_.step )
	    return true;
    }

    return false;
}


template <class VT> inline
void DataDistributionInfoExtracter<VT>::getCurve( SetType& xvals, SetType& yvals,
				     bool limitspikes ) const
{
    xvals.setEmpty(); yvals.setEmpty();

    if ( limitspikes )
    {
	RefMan< DataDistribution<VT> > despiked = distrib_.clone();
	DataDistributionChanger<VT>( *despiked ).deSpike();
	DataDistributionInfoExtracter<VT>( *despiked ).getCurve(
					    xvals, yvals, false );
    }
    else
    {
	DataDistributionIter<VT> iter( distrib_ );
	while ( iter.next() )
	{
	    xvals += iter.position();
	    yvals += iter.value();
	}
    }
}


template <class VT> inline
void DataDistributionInfoExtracter<VT>::getRanges( Interval<pos_type>& xrg,
				      Interval<pos_type>& yrg ) const
{
    mLockMonitorable4Read( distrib_ );
    xrg.start = distrib_.sampling_.start;
    const idx_type sz = distrib_.data_.size();
    if ( sz < 1 )
    {
	xrg.stop = xrg.start;
	yrg.start = yrg.stop = 0;
    }
    else
    {
	xrg.stop = distrib_.sampling_.atIndex( sz-1 );
	yrg.start = yrg.stop = distrib_.data_[0];
	for ( idx_type idx=1; idx<sz; idx++ )
	{
	    const VT val = distrib_.data_[idx];
	    if ( val > yrg.stop )
		yrg.stop = val;
	    else if ( val < yrg.start )
		yrg.start = val;
	}
    }
}


template <class VT> inline
void DataDistributionInfoExtracter<VT>::getAvgStdRMS( pos_type& avg,
				    pos_type& stdev, pos_type& rms ) const
{
    mLockMonitorable4Read( distrib_ );
    const int sz = distrib_.data_.size();
    if ( sz < 1 )
	{ avg = rms = stdev = pos_type(0); return; }
    avg = rms = distrib_.data_[0]; stdev = pos_type(0);
    if ( sz == 1 )
	return;

    // TODO: not entirely correct but no disaster
    // The problem is a.o. that within a bin the distrib is not uniform.
    // "Sheppard's correction".

    VT sum_x, sum_xx, sum_w, sum_wx, sum_wxx;
    sum_x = sum_xx = sum_w = sum_wx = sum_wxx = VT(0);

    for ( idx_type idx=0; idx<sz; idx++ )
    {
	const VT x = distrib_.sampling_.atIndex( idx );
	const VT wt = distrib_.data_[idx];
	sum_x += x; sum_xx += x * x;
	sum_w += wt; sum_wx += wt * x; sum_wxx += wt * x * x;
    }

    if ( sum_w == 0 )
	return;

    avg = sum_wx / sum_w;
    rms = Math::Sqrt( sum_wxx / sum_w );
    const VT var = (sum_wxx - (sum_wx * sum_wx / sum_w))
		 / ( ((sz-1) * sum_w) / sz );
    stdev = var > VT(0) ? Math::Sqrt( var ) : VT(0);
}


template <class VT> inline
void DataDistributionInfoExtracter<VT>::fillPar( IOPar& iop ) const
{
    mLockMonitorable4Read( distrib_ );
    iop.set( sKey::Sampling(), distrib_.sampling_ );
    iop.set( sKey::Data(), distrib_.data_ );
}


template <class VT> inline
void DataDistributionChanger<VT>::usePar( const IOPar& iop )
{
    mLockMonitorable4Write( distrib_ );
    iop.get( sKey::Sampling(), distrib_.sampling_ );
    iop.get( sKey::Data(), distrib_.data_ );
    distrib_.cumdata_.setSize( distrib_.data_.size() );
    for ( int idx=0; idx<distrib_.cumdata_.size(); idx++ )
	distrib_.setCumData( idx );
    mSendMonitorableEntireObjChgNotif( distrib_ );
}
