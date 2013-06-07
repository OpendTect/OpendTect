#ifndef convert_h
#define convert_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id$
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

template <>
inline void set( const char*& _to, const short& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const unsigned short& i )
    { _to = toString(i); }


#define mConvDefFromStrToFn(type,function) \
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

mConvDefFromStrToFn( int, (int)strtol(s,&endptr,0) )
mConvDefFromStrToFn( od_uint32, (od_uint32)strtoul(s,&endptr,0) )
mConvDefFromStrToFn( od_int64, strtoll(s,&endptr,0) )
mConvDefFromStrToFn( od_uint64, strtoull(s,&endptr,0) )
mConvDefFromStrToFn( double, strtod(s,&endptr) )
mConvDefFromStrToFn( float, strtof(s,&endptr) )

#undef mConvDefFromStrToFn


#define mConvDefFromStrToFn(type,fn) \
template <> \
inline void set( type& _to, const char* const& s ) \
    { _to = (type)fn(s); }


mConvDefFromStrToFn( short, atoi )
mConvDefFromStrToFn( unsigned short, atoi )

#undef mConvDefFromStrToFn

    
template <>
inline void set( bool& _to, const char* const& s )
    { _to = yesNoFromString(s); }

template <>
inline void set( od_int32& _to, const float& f )
    { _to = mRounded(od_int32,f); }

template <>
inline void set( od_int64& _to, const float& f )
    { _to = mRounded(od_int64,f); }

template <>
inline void set( short& _to, const float& f )
    { _to = mRounded(short,f); }

template <>
inline void set( unsigned short& _to, const float& f )
    { _to = mRounded(unsigned short,f); }

template <>
inline void set( od_uint32& _to, const float& f )
    { _to = mRounded(od_uint32,f); }

template <>
inline void set( od_uint64& _to, const float& f )
    { _to = mRounded(od_uint64,f); }

template <>
inline void set( od_int32& _to, const double& f )
    { _to = mRounded(od_int32,f); }

template <>
inline void set( od_int64& _to, const double& f )
    { _to = mRounded(od_int64,f); }

template <>
inline void set( short& _to, const double& f )
    { _to = mRounded(short,f); }

template <>
inline void set( unsigned short& _to, const double& f )
    { _to = mRounded(unsigned short,f); }

template <>
inline void set( od_uint32& _to, const double& f )
    { _to = mRounded(od_uint32,f); }

template <>
inline void set( od_uint64& _to, const double& f )
    { _to = mRounded(od_uint64,f); }

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
