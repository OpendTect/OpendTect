#ifndef undefval_h
#define undefval_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: undefval.h,v 1.18 2012-07-24 18:40:53 cvskris Exp $
________________________________________________________________________

-*/

#include "commondefs.h"
#include "plftypes.h"

//! Undefined value. IEEE gives NaN but that's not exactly what we want
#define __mUndefValue             1e30
//! Check on undefined. Also works when double converted to float and vv
#define __mIsUndefinedD(x)         (((x)>9.99999e29)&&((x)<1.00001e30))
#define __mIsUndefinedF(x)         (((x)>9.99999e29f)&&((x)<1.00001e30f))
//! Almost MAXINT so unlikely, but not MAXINT to avoid that
#define __mUndefIntVal            2109876543
//! Almost MAXINT64 therefore unlikely.
#define __mUndefIntVal64          9223344556677889900LL


#ifdef __cpp__


/*!  \brief Templatized undefined and initialisation (i.e. null) values.  

    Since these are all templates, they can be used much more generic
    than previous solutions with macros.

Use like:

  T x = mUdf(T);
  if ( mIsUdf(x) )
      mSetUdf(y);

*/

namespace Values
{

/*!  \brief Templatized undefined values.  */
template<class T>
class Undef
{
public:
    static T		val();
    static bool		hasUdf();
    static bool		isUdf(T);
    void		setUdf(T&);
};


template<>
class Undef<od_int16>
{
public:
    static od_int16	val()			{ return -32767; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( od_int32 i )	{ return i == -32767; }
    static void		setUdf( od_int32& i )	{ i = -32767; }
};

template<>
class Undef<od_uint16>
{
public:
    static od_uint16	val()			{ return 65534; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( od_uint32 i )	{ return i == 65534; }
    static void		setUdf( od_uint32& i )	{ i = 65534; }
};

template<>
class Undef<od_int32>
{
public:
    static od_int32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_int32 i )	{ return i == __mUndefIntVal; }
    static void		setUdf( od_int32& i )	{ i = __mUndefIntVal; }
};

template<>
class Undef<od_uint32>
{
public:
    static od_uint32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_uint32 i )	{ return i == __mUndefIntVal; }
    static void		setUdf( od_uint32& i )	{ i = __mUndefIntVal; }
};


template<>
class Undef<od_int64>
{
public:
    static od_int64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_int64 i )	{ return i == __mUndefIntVal64;}
    static void		setUdf( od_int64& i )	{ i = __mUndefIntVal64; }
};

template<>
class Undef<od_uint64>
{
public:
    static od_uint64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( od_uint64 i )	{ return i == __mUndefIntVal64;}
    static void		setUdf( od_uint64& i )	{ i = __mUndefIntVal64; }
};


template<>
class Undef<bool>
{
public:
    static bool		val()			{ return false; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( bool b )		{ return false; }
    static void		setUdf( bool& b )	{ b = false; }
};


template<>
class Undef<float>
{
public:
    static float	val()			{ return (float)__mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( float f )	{ return __mIsUndefinedF(f); }
    static void		setUdf( float& f )	{ f = (float)__mUndefValue; }
};


template<>
class Undef<double>
{
public:
    static double	val()			{ return __mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( double d )	{ return __mIsUndefinedD(d); }
    static void		setUdf( double& d )	{ d = __mUndefValue; }
};


template<>
class Undef<const char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( const char*& )	{}
};


template<>
class Undef<char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( char*& s )	{ if ( s ) *s = '\0'; }
};


template <class T> inline
bool isUdf( const T& t )
{ 
    return Undef<T>::isUdf(t);  
}

template <class T> inline
const T& udfVal( const T& t )
{ 
    static T u = Undef<T>::val();
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
inline bool dbgIsUdf( T val )
    { return Values::isUdf( val ); }
mGlobal bool dbgIsUdf(float);
mGlobal bool dbgIsUdf(double);

#ifdef __debug__
# define mIsUdf(val) dbgIsUdf(val)
#else
//! Use mIsUdf to check for undefinedness of simple types
# define mIsUdf(val) Values::isUdf(val)
#endif


#else

/* for C, fallback to old style macro's. Do not provide mUdf and mIsUdf to
   ensure explicit thinking about the situation. */

# define scUndefValue		 "1e30"
# define mcUndefValue             __mUndefValue
# define mcIsUndefined(x)         __mIsUndefinedD(x)


#endif 


#endif
