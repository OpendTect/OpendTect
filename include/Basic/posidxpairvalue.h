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

template <class FT> class IdxPairValues;


/*!\brief IdxPair with a value. */

template <class FT>
mClass(Basic) ValueIdxPair : public IdxPair
{
public:

    typedef FT	value_type;

    inline	ValueIdxPair( int f=0, int s=0, FT v=mUdf(FT) )
		: IdxPair(f,s), value_(v)	{}
    inline	ValueIdxPair( const IdxPair& b, FT v=mUdf(FT) )
		: IdxPair(b), value_(v)		{}
    inline	ValueIdxPair(const IdxPairValues<FT>&,int idx=0);

    inline bool	operator==(const ValueIdxPair&) const;
    inline bool	operator!=( const ValueIdxPair& vip ) const
						{ return !(*this == vip); }
    inline bool	equalPos( const IdxPair& ip ) const
					     { return IdxPair::operator==(ip); }

    inline	operator FT&()			{ return value_; }
    inline	operator FT() const		{ return value_; }

    inline bool	isUdf() const			{ return IdxPair::isUdf()
						      || mIsUdf(value_); }

    FT		value_;

};	



/*!\brief IdxPair with 0-N values.

If one of the values is Z, make it the first one.
*/

template <class FT>
mExpClass(Basic) IdxPairValues : public IdxPair
{
public:

    typedef typename TypeSet<FT>::size_type size_type;
    typedef FT		value_type;

			IdxPairValues()		{ setSize(0); }
			IdxPairValues( int i1, int i2, int nrvals )
			    : IdxPair(i1,i2)	{ setSize(nrvals); }
			IdxPairValues( const IdxPair& p, int nrvals )
			    : IdxPair(p)	{ setSize(nrvals); }
			IdxPairValues( const IdxPairValues& oth )
			    : IdxPair()		{ *this = oth; }
			IdxPairValues( const ValueIdxPair<FT>& vip )
			    : IdxPair(vip)	{ setSize(1); value(0) = vip; }

    inline bool		operator==(const IdxPairValues&) const;
    inline bool		operator!=( const IdxPairValues& ipvs ) const
						{ return !(*this == ipvs); }
    inline bool		equalPos( const IdxPair& ip ) const
					     { return IdxPair::operator==(ip); }

    inline size_type	size() const		{ return vals_.size(); }
    inline FT&		value( int idx )	{ return vals_[idx]; }
    inline FT		value( int idx ) const	{ return vals_[idx]; }
    inline FT*		values()		{ return vals_.arr(); }
    inline const FT*	values() const		{ return vals_.arr(); }
    inline operator	FT*()			{ return values(); }
    inline operator	const FT*() const	{ return values(); }
    inline FT&		operator[]( int idx )	{ return value( idx ); }
    inline FT		operator[]( int idx ) const { return value( idx ); }

    inline void		setSize(size_type,bool kpvals=false);
    inline void		setVals(const FT*);

    inline bool		isUdf(bool allvalsudf=true) const;

    TypeSet<FT>	vals_;

};


template <class FT>
inline ValueIdxPair<FT>::ValueIdxPair( const IdxPairValues<FT>& ipvs, int idx )
    : IdxPair(ipvs), value_(ipvs.value(idx))
{
}


template <class FT>
inline bool ValueIdxPair<FT>::operator ==( const ValueIdxPair<FT>& oth ) const
{
    if ( !equalPos( oth ) )
	return false;
    else if ( mIsUdf(value_) && mIsUdf(oth.value_) )
	return true;

    const FT valdiff = value_ - oth.value;
    return mIsZero(valdiff,(FT)mDefEps);
}


template <class FT>
inline bool IdxPairValues<FT>::operator ==( const IdxPairValues<FT>& oth ) const
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


template <class FT>
inline void IdxPairValues<FT>::setSize( IdxPairValues<FT>::size_type sz,
				    bool kpvals )
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


template <class FT>
inline void IdxPairValues<FT>::setVals( const FT* newvals )
{
    const size_type sz = size();
    for ( size_type idx=0; idx<sz; idx++ )
	vals_[idx] = newvals ? newvals[idx] : mUdf(float);
}


template <class FT>
inline bool IdxPairValues<FT>::isUdf( bool anyvaludf ) const
{
    if ( IdxPair::isUdf() )
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
