#ifndef basictypes_h
#define basictypes_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: convert.h,v 1.1 2005-02-23 14:45:12 cvsarend Exp $
________________________________________________________________________

-*/

#include "undefval.h"
#include "string2.h"

namespace Conv{

//! template based type converstion
template <class T, class F>
inline void set( T& _to, const F& fr )
    { _to = fr; }

template <class T, class F>
inline T& to( const F& fr )
{ 
    static T ret;
    set<T,F>(ret,fr);

    return ret;
}


//! template based type converstion, with check for undef
template <class T, class F>
inline void udfset(T& _to, const F& fr, const T& und= Values::Undef<T>::val())
{
    if( Values::hasUdf<F>() && Values::isUdf(fr) )
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
    udfset<T,F>(ret,fr,und);

    return ret;
}


//----- specialisations

template <>
inline void set( const char*& _to, const int32& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const uint32& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const int64& i )
    { _to = toString(i); }

template <>
inline void set( const char*& _to, const uint64& i )
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
inline void set( int32& _to, const char* const& s )
{
    if ( !s || !*s ) { return; }

    char* endptr;  
    int32 tmpval = strtol(s, &endptr, 0);
    if ( s==endptr )
    {
       	if ( Values::Undef<int32>::hasUdf() ) Values::setUdf(_to); 
	return;
    }

    _to = tmpval; 
}

template <>
inline void set( bool& _to, const char* const& s )
    { _to = yesNoFromString(s); }

template <>
inline void set( double& _to, const char* const& s )
    { _to = atof(s); }

template <>
inline void set( float& _to, const char* const& s )
    { _to = atof(s); }

template <>
inline void set( int32& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( int64& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( int32& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( int64& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( uint32& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( uint64& _to, const float& f )
    { _to = mNINT(f); }

template <>
inline void set( uint32& _to, const double& f )
    { _to = mNINT(f); }

template <>
inline void set( uint64& _to, const double& f )
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
