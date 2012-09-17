#ifndef initval_h
#define initval_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: initval.h,v 1.3 2009/07/22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "plftypes.h"

namespace Values
{

/*! \brief Templatized initialisation values.  */
template<class T>
class Init
{
public:
    static T		val();
};


template<>
class Init<od_int16>
{
public:
    static od_int16	val()		{ return 0; }
};

template<>
class Init<od_uint16>
{
public:
    static od_uint16	val()		{ return 0; }
};


template<>
class Init<od_int32>
{
public:
    static od_int32	val()		{ return 0; }
};

template<>
class Init<od_uint32>
{
public:
    static od_uint32	val()		{ return 0; }
};


template<>
class Init<od_int64>
{
public:
    static od_int64	val()		{ return 0; }
};

template<>
class Init<od_uint64>
{
public:
    static od_uint64	val()		{ return 0; }
};


template<>
class Init<bool>
{
public:
    static bool		val()		{ return false; }
};


template<>
class Init<float>
{
public:
    static float	val()		{ return 0.0f; }
};


template<>
class Init<double>
{
public:
    static double	val()		{ return 0.0; }
};


template<>
class Init<const char*>
{
public:
    static const char*	val()			{ return ""; }
};


template <class T>
T& init( T& e )
{ 
    e = Init<T>::val(); 
    return e; 
}


template <class T>
const T& initVal()
{ 
    static T e= Init<T>::val();
    return e;
}

}

#endif
