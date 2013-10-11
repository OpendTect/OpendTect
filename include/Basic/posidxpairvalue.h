#ifndef posidxpairvalue_h
#define posidxpairvalue_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1996/Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxpair.h"
#include "typeset.h"


namespace Pos
{

template <class IPT,class FT> class IdxPairValues;


/*!\brief IdxPair with a value. */

template <class IPT,class FT>
mClass(Basic) ValueIdxPair : public IPT
{
public:

    typedef IPT	pos_type;
    typedef FT	value_type;

    inline	ValueIdxPair( int f=0, int s=0, FT v=mUdf(FT) )
		: IPT(f,s), val_(v)	{}
    inline	ValueIdxPair( const IPT& b, FT v=mUdf(FT) )
		: IPT(b), val_(v)		{}
    inline	ValueIdxPair( const IdxPairValues<IPT,FT>& ipvs, int idx=0 )
						{ set( ipvs, idx ); }

    inline bool	operator==(const ValueIdxPair&) const;
    inline bool	operator!=( const ValueIdxPair& vip ) const
						{ return !(*this == vip); }
    inline bool	equalPos( const IPT& ip ) const
						{ return IPT::operator==(ip); }

    FT&		val()				{ return val_; }
    FT		val() const			{ return val_; }
    inline	operator FT&()			{ return val_; }
    inline	operator FT() const		{ return val_; }

    inline void	set( FT f )			{ val_ = f; }
    inline void	set( const IPT& ipt )		{ IPT::operator=( ipt ); }
    inline void	set( const IPT& ipt, FT f )	{ set( ipt ); set( f ); }
    inline void	set(const IdxPairValues<IPT,FT>&,int idx=0);

    inline bool	isUdf() const			{ return IPT::isUdf()
						      || mIsUdf(val_); }

protected:

    FT		val_;

};	



/*!\brief IdxPair with 0-N values.

If one of the values is Z, make it the first one.
*/

template <class IPT,class FT>
mExpClass(Basic) IdxPairValues : public IPT
{
public:

    typedef IPT	pos_type;
    typedef FT	value_type;
    typedef TypeSet<FT> set_type;
    typedef typename TypeSet<FT>::size_type size_type;

			IdxPairValues()		{ setSize(0); }
			IdxPairValues( int i1, int i2, int nrvals )
			    : IPT(i1,i2)	{ setSize(nrvals); }
			IdxPairValues( const IPT& p, int nrvals )
			    : IPT(p)		{ setSize(nrvals); }
			IdxPairValues( const IdxPairValues& oth )
			    : IPT()		{ *this = oth; }
			IdxPairValues( const ValueIdxPair<IPT,FT>& vip )
			    : IPT(vip)		{ setSize(1); value(0) = vip; }

    inline bool		operator==(const IdxPairValues&) const;
    inline bool		operator!=( const IdxPairValues& ipvs ) const
						{ return !(*this == ipvs); }
    inline bool		equalPos( const IPT& ip ) const
						{ return IPT::operator==(ip); }

    inline bool		validIdx( int i ) const	{ return vals_.validIdx(i); }
    inline size_type	size() const		{ return vals_.size(); }
    inline FT&		value( int idx )	{ return vals_[idx]; }
    inline FT		value( int idx ) const	{ return vals_[idx]; }
    inline FT*		values()		{ return vals_.arr(); }
    inline const FT*	values() const		{ return vals_.arr(); }
    inline FT&		operator[]( int idx )	{ return value( idx ); }
    inline FT		operator[]( int idx ) const { return value( idx ); }
    inline set_type&	valSet()		{ return vals_; }
    inline const set_type& valSet() const	{ return vals_; }
    inline operator	FT*()			{ return values(); }
    inline operator	const FT*() const	{ return values(); }

    inline void		setSize(size_type,bool kpvals=false);
    inline void		setVals(const FT*);
    inline void		set(int,FT);
    inline void		set( const IPT& ipt )	{ IPT::operator=( ipt ); }

    inline bool		isUdf(bool allvalsudf=true) const;

protected:

    TypeSet<FT>	vals_;

};


template <class IPT,class FT>
inline void ValueIdxPair<IPT,FT>::set( const IdxPairValues<IPT,FT>& ipvs,
					   int idx )
{
    IPT::operator=( ipvs );
    val_ = ipvs.validIdx( idx ) ? ipvs.value( idx ) : mUdf(FT);
}


template <class IPT,class FT>
inline bool ValueIdxPair<IPT,FT>::operator ==( const ValueIdxPair<IPT,FT>& oth ) const
{
    if ( !equalPos( oth ) )
	return false;
    else if ( mIsUdf(val_) && mIsUdf(oth.val_) )
	return true;

    const FT valdiff = val_ - oth.val_;
    return mIsZero(valdiff,(FT)mDefEps);
}


template <class IPT,class FT>
inline bool IdxPairValues<IPT,FT>::operator ==(
				const IdxPairValues<IPT,FT>& oth ) const
{
    const size_type sz = size();
    if ( sz != size() || !equalPos( oth ) )
	return false;

    for ( size_type idx=0; idx<sz; idx++ )
    {
	const FT myval = value(idx);
	const FT othval = oth.value(idx);

	if ( mIsUdf(myval) && mIsUdf(othval) )
	    continue;

	const FT valdiff = myval - othval;
	if ( !mIsZero(valdiff,(FT)mDefEps) )
	    return false;
    }

    return true;
}


template <class IPT,class FT>
inline void IdxPairValues<IPT,FT>::setSize(
		typename IdxPairValues<IPT,FT>::size_type sz, bool kpvals )
{
    if ( sz == size() )
    {
	if ( kpvals )
	    return;
	vals_.erase();
    }

    TypeSet<FT> tmp( vals_ );
    const size_type orgsz = tmp.size();
    for ( size_type idx=0; idx<sz; idx++ )
	vals_ += idx<orgsz ? tmp[idx] : mUdf(float);
}


template <class IPT,class FT>
inline void IdxPairValues<IPT,FT>::setVals( const FT* newvals )
{
    const size_type sz = size();
    for ( size_type idx=0; idx<sz; idx++ )
	vals_[idx] = newvals ? newvals[idx] : mUdf(float);
}


template <class IPT,class FT>
inline void IdxPairValues<IPT,FT>::set( int idx, FT val )
{
    if ( validIdx(idx) )
	vals_[idx] = val;
}


template <class IPT,class FT>
inline bool IdxPairValues<IPT,FT>::isUdf( bool anyvaludf ) const
{
    if ( IPT::isUdf() )
	return true;

    const size_type sz = size();
    for ( size_type idx=0; idx<sz; idx++ )
    {
	const FT val = vals_[idx];
	const bool isudf = mIsUdf(val);
	if ( isudf && anyvaludf )
	    return true; // any val needs to be udf and this one is
	else if ( !isudf && !anyvaludf )
	    return false; // all vals need to be udf and this one isn't
    }

    return !anyvaludf;
}


} // namespace Pos


#endif
