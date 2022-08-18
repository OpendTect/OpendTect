#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "undefval.h"
#include "bufstring.h"

#ifdef __msvc__
# include "msvcdefs.h"
#endif

/*!\brief Template based type conversion. */

namespace Conv
{

//! template based type conversion
template <class T, class F>
inline void set( T& _to, const F& fr )
    { _to = sCast(T,fr); }

//! convenience to be able to code something like:
//! val = Conv::to<the_type_of_val>( x );
//! if conversion fails, val will be set to the appropriate undef.
template <class T, class F>
inline T to( const F& fr )
{
    T ret;
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
inline T udfto( const F& fr, const T& und = Values::Undef<T>::val() )
{
    T ret;
    Values::setUdf(ret);
    udfset<T,F>(ret,fr,und);

    return ret;
}


//----- specializations 1: simple types -> const char*

template <>
inline void set( const char*& _to, const short& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const unsigned short& i )
    { _to = toString(i); }

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


//----- specializations 2: floating point types -> integer types

template <>
inline void set( short& _to, const float& f )
    { _to = mRounded(short,f); }

template <>
inline void set( unsigned short& _to, const float& f )
    { _to = mRounded(od_uint16,f); }

template <>
inline void set( od_int32& _to, const float& f )
    { _to = mRounded(od_int32,f); }

template <>
inline void set( od_uint32& _to, const float& f )
    { _to = mRounded(od_uint32,f); }

template <>
inline void set( od_int64& _to, const float& f )
    { _to = mRounded(od_int64,f); }

template <>
inline void set( od_uint64& _to, const float& f )
    { _to = mRounded(od_uint64,f); }

template <>
inline void set( short& _to, const double& f )
    { _to = mRounded(short,f); }

template <>
inline void set( unsigned short& _to, const double& f )
    { _to = mRounded(od_uint16,f); }

template <>
inline void set( od_int32& _to, const double& f )
    { _to = mRounded(od_int32,f); }

template <>
inline void set( od_uint32& _to, const double& f )
    { _to = mRounded(od_uint32,f); }

template <>
inline void set( od_int64& _to, const double& f )
    { _to = mRounded(od_int64,f); }

template <>
inline void set( od_uint64& _to, const double& f )
    { _to = mRounded(od_uint64,f); }


//----- specializations 3: strings and simple types -> bool

template <>
inline void set( bool& _to, const char* const& s )
    { _to = yesNoFromString(s); }

template <>
inline void set( bool& _to, const StringView& s )
    { _to = yesNoFromString(s.str()); }

template <>
inline void set( bool& _to, const BufferString& s )
    { _to = yesNoFromString(s.str()); }

template <>
inline void set( bool& _to, const int& i )
    { _to = i!=0; }

template <>
inline void set( bool& _to, const float& f )
    { _to = !mIsZero(f,mDefEpsF); }

template <>
inline void set( bool& _to, const double& d )
    { _to = !mIsZero(d,mDefEpsD); }


//----- specializations 4: strings -> simple types


#define mConvDeclFromStrToSimpleType(type) \
template <> mGlobal(Basic) void set(type&,const char* const&); \
template <> mGlobal(Basic) void set(type&,const StringView&); \
template <> mGlobal(Basic) void set(type&,const BufferString&)

mConvDeclFromStrToSimpleType(short);
mConvDeclFromStrToSimpleType(unsigned short);
mConvDeclFromStrToSimpleType(int);
mConvDeclFromStrToSimpleType(od_uint32);
mConvDeclFromStrToSimpleType(od_int64);
mConvDeclFromStrToSimpleType(od_uint64);
mConvDeclFromStrToSimpleType(double);
mConvDeclFromStrToSimpleType(float);

} // namespace Conv


//! macro useful for implementation of string -> your type

#define mConvDefFromStrToSimpleType(type,function) \
namespace Conv \
{ \
    template <> void set( type& _to, const char* const& s ) \
    { \
	if ( !s || !*s ) { return; } \
    \
	char* endptr = nullptr; \
	type tmpval = sCast(type,function); \
	if ( s != endptr ) \
	    _to = sCast(type,tmpval); \
	else if ( Values::Undef<type>::hasUdf() ) \
	    Values::setUdf( _to ); \
    } \
    template <> void set( type& _to, const StringView& s ) \
    { \
	if ( s.isEmpty() ) { return; } \
    \
	char* endptr = nullptr; \
	type tmpval = sCast(type,function); \
	if ( s.str() != endptr ) \
	    _to = sCast(type,tmpval); \
	else if ( Values::Undef<type>::hasUdf() ) \
	    Values::setUdf( _to ); \
    } \
    template <> void set( type& _to, const BufferString& s ) \
    { \
	if ( s.isEmpty() ) { return; } \
    \
	char* endptr = nullptr; \
	type tmpval = sCast(type,function); \
	if ( s.str() != endptr ) \
	    _to = sCast(type,tmpval); \
	else if ( Values::Undef<type>::hasUdf() ) \
	    Values::setUdf( _to ); \
    } \
}
