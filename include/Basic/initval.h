#ifndef initval_h
#define initval_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: initval.h,v 1.1 2005-02-23 14:45:12 cvsarend Exp $
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
class Init<int16>
{
public:
    static int16	val()		{ return 0; }
};

template<>
class Init<uint16>
{
public:
    static uint16	val()		{ return 0; }
};


template<>
class Init<int32>
{
public:
    static int32	val()		{ return 0; }
};

template<>
class Init<uint32>
{
public:
    static uint32	val()		{ return 0; }
};


template<>
class Init<int64>
{
public:
    static int64	val()		{ return 0; }
};

template<>
class Init<uint64>
{
public:
    static uint64	val()		{ return 0; }
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
