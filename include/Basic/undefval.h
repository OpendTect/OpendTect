#ifndef undefval_h
#define undefval_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: undefval.h,v 1.1 2005-02-23 14:45:12 cvsarend Exp $
________________________________________________________________________

-*/

#include "plftypes.h"

#ifdef __cpp__
# include "keystrs.h"
#endif

#define __sUndefValue		  sKey::FloatUdf
#define __mUndefValue             1e30
#define __mIsUndefined(x)         (((x)>9.99999e29)&&((x)<1.00001e30))
#define __mUndefIntVal            2109876543
#define __mUndefIntVal64          9223344556677889900LL
#define __mIsUndefInt(x)          ((x) == __mUndefIntVal)

#ifndef __cpp__

/* for C, fallback to old style macro's */

#define csUndefValue		 "1e30"
#define mcUndefValue             __mUndefValue
#define mcIsUndefined(x)         __mIsUndefined(x)
#define mcUndefIntVal            __mUndefIntVal
#define mcUndefIntVal64          __mUndefIntVal64
#define mcIsUndefInt(x)          __mIsUndefInt(x)

#else

#define mUdf(type) Values::Undef<type>::val()

namespace Values
{
/*! \brief Templatized undefined values.  */

template<class T>
class Undef
{
public:
    static T		val();
    static bool		hasUdf();
    static bool		isUdf( T );
};


template<>
class Undef<int32>
{
public:
    static int32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( int32 i )	{ return i==__mUndefIntVal; }
};

template<>
class Undef<uint32>
{
public:
    static uint32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( uint32 i )	{ return i==__mUndefIntVal; }
};


template<>
class Undef<int64>
{
public:
    static int64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( int64 i )	{ return i==__mUndefIntVal64; }
};

template<>
class Undef<uint64>
{
public:
    static uint64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( uint64 i )	{ return i==__mUndefIntVal64; }
};


template<>
class Undef<bool>
{
public:
    static bool		val()			{ return false; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( bool b )	{ return false; }
};


template<>
class Undef<float>
{
public:
    static float	val()			{ return __mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( float f )	{ return __mIsUndefined(f); }
};


template<>
class Undef<double>
{
public:
    static double	val()			{ return __mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( double d )	{ return __mIsUndefined(d); }
};


template<>
class Undef<const char*>
{
public:
    static const char*	val()				{ return ""; }
    static bool		hasUdf()			{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
};


template <class T>
T& setUdf( T& u )
{ 
    u = Undef<T>::val(); 
    return u; 
}


template <class T>
bool isUdf( const T& t)
{ 
    return Undef<T>::isUdf(t);  
}



template <class T>
const T& udfVal( const T& t )
{ 
    static T u = Undef<T>::val();
    return u;
}

template <class T>
const T& val()
{ 
    static T u = Undef<T>::val();
    return u;
}

template <class T>
bool hasUdf()
{ 
    return Undef<T>::hasUdf();  
}

}
#endif 
#endif
