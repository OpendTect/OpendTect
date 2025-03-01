#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"
#include "undefval.h"
#include <utility>

namespace OD
{

template <class T1,class T2>
mClass(Basic) Pair
{
public:
			Pair() {}
			Pair(const T1&,const T2&);
    virtual		~Pair();

    bool		operator==(const Pair<T1,T2>&) const;

    void		set(const T1&,const T2&);
    T1&			first()		{ return pair_.first; }
    T2&			second()	{ return pair_.second; }
    const T1&		first() const	{ return pair_.first; }
    const T2&		second() const	{ return pair_.second; }

    void		setUdf();
    bool		isUdf() const;
    static Pair<T1,T2>	udf();

    std::pair<T1,T2>	pair_;
};


template <class T1,class T2>
Pair<T1,T2>::Pair( const T1& v1, const T2& v2 )
{
    pair_.first = v1;
    pair_.second = v2;
}


template <class T1,class T2>
Pair<T1,T2>::~Pair()
{}


template <class T1,class T2>
bool Pair<T1,T2>::operator==( const Pair<T1,T2>& oth ) const
{
    return pair_ == oth.pair_;
}


template <class T1,class T2>
void Pair<T1,T2>::set( const T1& v1, const T2& v2 )
{
   pair_.first = v1;
   pair_.second = v2;
}


template <class T1,class T2>
void Pair<T1,T2>::setUdf()
{
   set( mUdf(T1), mUdf(T2) );
}


template <class T1,class T2>
bool Pair<T1,T2>::isUdf() const
{
    return mIsUdf(pair_.first) && mIsUdf(pair_.second);
}


template <class T1,class T2>
Pair<T1,T2> Pair<T1,T2>::udf()
{
    return Pair<T1,T2>( mUdf(T1), mUdf(T2) );
}

// For convenience
using PairUI16 = Pair<od_int16,od_uint16>;
using PairUI32 = Pair<od_int32,od_uint32>;
using PairUI64 = Pair<od_int64,od_uint64>;
using PairI16 = Pair<od_int16,od_int16>;
using PairI32 = Pair<od_int32,od_int32>;
using PairI64 = Pair<od_int64,od_int64>;
using PairF = Pair<float,float>;
using PairD = Pair<double,double>;

} // namespace OD
