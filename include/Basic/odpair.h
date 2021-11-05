#pragma once
/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commanddefs.h"
#include <utility>

namespace OD
{

template <class T1,class T2>
mClass(Basic) Pair
{
public:
			Pair() {}
			Pair(const T1&,const T2&);

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

} // namespace OD
