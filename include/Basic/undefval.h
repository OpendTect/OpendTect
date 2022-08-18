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

//! Undefined value. IEEE gives NaN but that's not exactly what we want
#define __mUndefLDValue           1e30L
#define __mUndefDValue            1e30
#define __mUndefFValue            1e30f
//! Check on undefined. Also works when double converted to float and vv
#define __mIsUndefinedLD(x)        (((x)>9.99999e29L)&&((x)<1.00001e30L))
#define __mIsUndefinedD(x)         (((x)>9.99999e29)&&((x)<1.00001e30))
#define __mIsUndefinedF(x)         (((x)>9.99999e29f)&&((x)<1.00001e30f))
#define __mIsUndefinedI(x,udfval)  (((x)>=udfval)||((x)<=-udfval))
//! Almost MAXINT so unlikely, but not MAXINT to avoid that
#define __mUndefIntVal            2109876543
//! Almost MAXINT64 therefore unlikely.
#define __mUndefIntVal64          9223344556677889900LL


/*!  \brief Templatized undefined and initialization (i.e. null) values.

    Since these are all templates, they can be used much more generic
    than previous solutions with macros.

Use like:

  T x = mUdf(T);
  if ( mIsUdf(x) )
      mSetUdf(y);

*/

namespace Values
{

/*!
\brief Templatized undefined values.
*/

template<class T>
mClass(Basic) Undef
{
public:
    static T		val();
    static bool		hasUdf();
    static bool		isUdf(T);
    void		setUdf(T&);
};


/*!
\brief Undefined od_int16.
*/

template<>
mClass(Basic) Undef<od_int16>
{
public:
    static od_int16	val()			{ return 32766; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( od_int16 i )	{ return i == 32766; }
    static void		setUdf( od_int16& i )	{ i = 32766; }
};


/*!
\brief Undefined od_uint16.
*/

template<>
mClass(Basic) Undef<od_uint16>
{
public:
    static od_uint16	val()			{ return 65534; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( od_uint16 i )	{ return i == 65534; }
    static void		setUdf( od_uint16& i )	{ i = 65534; }
};


/*!
\brief Undefined od_int32.
*/

template<>
mClass(Basic) Undef<od_int32>
{
public:
    static od_int32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_int32 i )
			{ return i>=__mUndefIntVal || i<=-__mUndefIntVal; }
    static void		setUdf( od_int32& i )	{ i = __mUndefIntVal; }
};


/*!
\brief Undefined od_uint32.
*/

template<>
mClass(Basic) Undef<od_uint32>
{
public:
    static od_uint32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_uint32 i )	{ return i >= __mUndefIntVal; }
    static void		setUdf( od_uint32& i )	{ i = __mUndefIntVal; }
};


/*!
\brief Undefined od_int64.
*/

template<>
mClass(Basic) Undef<od_int64>
{
public:
    static od_int64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_int64 i )
			{ return i>=__mUndefIntVal64 || i<=-__mUndefIntVal64; }
    static void		setUdf( od_int64& i )	{ i = __mUndefIntVal64; }
};


/*!
\brief Undefined od_uint64.
*/

template<>
mClass(Basic) Undef<od_uint64>
{
public:
    static od_uint64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_uint64 i )	{ return i >= __mUndefIntVal64;}
    static void		setUdf( od_uint64& i )	{ i = __mUndefIntVal64; }
};


/*!
\brief Undefined bool.
*/

template<>
mClass(Basic) Undef<bool>
{
public:
    static bool		val()			{ return false; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( bool )		{ return false; }
    static void		setUdf( bool& b )	{ b = false; }
};


/*!
\brief Undefined float.
*/

template<>
mClass(Basic) Undef<float>
{
public:
    static float	val()			{ return __mUndefFValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( float f )	{ return __mIsUndefinedF(f); }
    static void		setUdf( float& f )	{ f = __mUndefFValue; }
};


/*!
\brief Undefined double.
*/

template<>
mClass(Basic) Undef<double>
{
public:
    static double	val()			{ return __mUndefDValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( double d )	{ return __mIsUndefinedD(d); }
    static void		setUdf( double& d )	{ d = __mUndefDValue; }
};


/*!
\brief Undefined long double.
*/

template<>
mClass(Basic) Undef<long double>
{
public:
    static long double	val()			{ return __mUndefLDValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( long double d )	{ return __mIsUndefinedLD(d); }
    static void		setUdf( long double& d ) { d = __mUndefLDValue; }
};


/*!
\brief Undefined const char*.
*/

template<>
mClass(Basic) Undef<const char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( const char*& )	{}
};


/*!
\brief Undefined char*.
*/

template<>
mClass(Basic) Undef<char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( char*& s )	{ if ( s ) *s = '\0'; }
};


/*!
\brief Undefined char.
*/

template<>
mClass(Basic) Undef<char>
{
public:
    static char		val()			{ return -127; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char s )	{ return s==-127; }
    static void		setUdf( char& s )	{ s = -127; }
};



template <class T> inline
bool isUdf( const T& t )
{
    return Undef<T>::isUdf(t);
}

template <class T> inline
const T& udfVal( const T& )
{
    mDefineStaticLocalObject( T, u, = Undef<T>::val() );
    return u;
}

template <class T> inline
bool hasUdf()
{
    return Undef<T>::hasUdf();
}

template <class T> inline
T& setUdf( T& u )
{
    Undef<T>::setUdf( u );
    return u;
}

}


//! Use this macro to get the undefined for simple types
#define mUdf(type) Values::Undef<type>::val()
//! Use this macro to set simple types to undefined
#define mSetUdf(val) Values::setUdf(val)



template <class T>
inline bool isUdfImpl( T val )
    { return Values::isUdf( val ); }

mGlobal(Basic) bool isUdfImpl(float);
mGlobal(Basic) bool isUdfImpl(double);


//! Use mIsUdf to check for undefinedness of simple types
# define mIsUdf(val) isUdfImpl(val)


/*! Only use this macro if speed really counts, e.g. when doing relatively
    simple operations on a huge range of data elements in a parallel task.
    It minimizes slowness from function calls and Math::IsNormalNumber(.).
    (eventual NaNs in C++ comparisons are said to return always false) */
#define mFastMaxReasonableFloat 1e20f
#define mFastIsFloatDefined(fval) \
    ( (fval>-mFastMaxReasonableFloat && fval<mFastMaxReasonableFloat) || \
      (!__mIsUndefinedF(fval) && Math::IsNormalNumber(fval)) )
