#ifndef refpair_h
#define refpair_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2011
 RCS:		$Id: refpair.h,v 1.2 2011/10/06 15:15:57 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

/* Defines a class holding a pair of references of different classes */

#define mDefRefPairClass(clssnm,T1,membnm1,T2,membnm2) \
class clssnm \
{ \
public: \
 \
    inline		clssnm( T1& r1, T2& r2 ) \
				: r1_(&r1), r2_(&r2)		{} \
    inline		clssnm( clssnm& oth ) \
				: r1_(oth.r1_), r2_(oth.r2_)	{} \
 \
    inline clssnm&	operator =( clssnm& oth ) \
			{ r1_ = oth.r1_; r2_ = oth.r2_; return *this; } \
    inline clssnm&	operator =( T1& r ) \
			{ r1_ = &r; return *this; } \
    inline clssnm&	operator =( T2& r ) \
			{ r2_ = &r; return *this; } \
 \
    inline operator	T1&() const			{ return *r1_; } \
    inline operator	T2&() const			{ return *r2_; } \
 \
    inline T1&		membnm1() const			{ return *r1_; } \
    inline T2&		membnm2() const			{ return *r2_; } \
    void		set( T1& r )			{ r1_ = &r; } \
    void		set( T2& r )			{ r2_ = &r; } \
 \
    T1*			r1_; \
    T2*			r2_; \
 \
}


/* Defines a class holding a pair of references of the same class */

#define mDefRefPairSameClass(clssnm,T) \
class clssnm \
{ \
public: \
 \
    inline		clssnm( T& r1, T& r2 ) \
				: r1_(&r1), r2_(&r2)		{} \
    inline		clssnm( clssnm& oth ) \
				: r1_(oth.r1_), r2_(oth.r2_)	{} \
 \
    inline clssnm&	operator =( clssnm& oth ) \
			{ r1_ = oth.r1_; r2_ = oth.r2_; return *this; } \
 \
    inline T&		first() const			{ return *r1_; } \
    inline T&		second() const			{ return *r2_; } \
    void		setFirst( T& r )		{ r1_ = &r; } \
    void		setSecond( T& r )		{ r2_ = &r; } \
 \
    T*			r1_; \
    T*			r2_; \
 \
}


#endif
