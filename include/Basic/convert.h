#ifndef convert_h
#define convert_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: convert.h,v 1.7 2007-10-05 11:28:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "undefval.h"
#include "string2.h"

#ifdef __msvc__
# include "msvcdefs.h"
#endif

namespace Conv{

//! template based type converstion
template <class T, class F>
inline void set( T& _to, const F& fr )
    { _to = fr; }

template <class T, class F>
inline T& to( const F& fr )
{ 
    static T ret;
    Values::setUdf(ret);
    set<T,F>(ret,fr);

    return ret;
}


//! template based type converstion, with check for undef
template <class T, class F>
inline void udfset(T& _to, const F& fr, const T& und= Values::Undef<T>::val())
{
    if ( Values::hasUdf<F>() && Values::isUdf(fr) )
	_to = und;
    else 
    {
	set(_to,fr);
	if ( Values::isUdf(_to) ) _to = und;
    }
}

template <class T, class F>
inline T& udfto( const F& fr, const T& und = Values::Undef<T>::val() )
{ 
    static T ret;
    Values::setUdf(ret);
    udfset<T,F>(ret,fr,und);

    return ret;
}


//----- specialisations

template <>
inline void set( const char*& _to, const od_int32& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const od_uint32& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const od_int64& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const od_uint64& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const bool& b )
    { _to = toString(b); }

template <>
inline void set( const char*& _to, const float& f )
    { _to = toString(f); }

template <>
inline void set( const char*& _to, const double& d )
    { _to = toString(d); }


#define mSetFromStrTo(type,function) \
template <> \
inline void set( type& _to, const char* const& s ) \
{ \
    if ( !s || !*s ) { return; } \
\
    char* endptr; \
    type tmpval = function; \
    if ( s != endptr ) \
	_to = tmpval; \
    else if ( Values::Undef<type>::hasUdf() ) \
	    Values::setUdf( _to ); \
} 

mSetFromStrTo( int, strtol(s, &endptr, 0) );
mSetFromStrTo( od_uint32, strtoul(s, &endptr, 0) );
mSetFromStrTo( od_int64, strtoll(s, &endptr, 0) );
mSetFromStrTo( od_uint64, strtoull(s, &endptr, 0) );
mSetFromStrTo( double, strtod(s, &endptr ) );
mSetFromStrTo( float, strtod(s, &endptr ) );

    
template <>
inline void set( bool& _to, const char* const& s )
    { _to = yesNoFromString(s); }

template <>
inline void set( od_int32& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( od_int64& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( od_int32& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( od_int64& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( od_uint32& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( od_uint64& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( od_uint32& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( od_uint64& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( bool& _to, const int& i )
    { _to = i!=0; }

template <>
inline void set( bool& _to, const float& f )
    { _to = !mIsZero(f,mDefEps); }

template <>
inline void set( bool& _to, const double& d )
    { _to = !mIsZero(d,mDefEps); }


}
#endif
