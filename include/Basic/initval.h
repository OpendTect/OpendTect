#ifndef initval_h
#define initval_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "plftypes.h"

namespace Values
{

/*!
\brief Templatized initialization values.
*/

template<class T>
mClass(Basic) Init
{
public:
    static T		val();
};


/*!
\brief Templatized od_int16 initialization values.
*/

template<>
mClass(Basic) Init<od_int16>
{
public:
    static od_int16	val()		{ return 0; }
};


/*!
\brief Templatized od_uint16 initialization values.
*/

template<>
mClass(Basic) Init<od_uint16>
{
public:
    static od_uint16	val()		{ return 0; }
};


/*!
\brief Templatized od_int32 initialization values.
*/

template<>
mClass(Basic) Init<od_int32>
{
public:
    static od_int32	val()		{ return 0; }
};


/*!
\brief Templatized od_uint32 initialization values.
*/

template<>
mClass(Basic) Init<od_uint32>
{
public:
    static od_uint32	val()		{ return 0; }
};


/*!
\brief Templatized od_int64 initialization values.
*/

template<>
mClass(Basic) Init<od_int64>
{
public:
    static od_int64	val()		{ return 0; }
};


/*!
\brief Templatized od_uint64 initialization values.
*/

template<>
mClass(Basic) Init<od_uint64>
{
public:
    static od_uint64	val()		{ return 0; }
};


/*!
\brief Templatized boolean initialization values.
*/

template<>
mClass(Basic) Init<bool>
{
public:
    static bool		val()		{ return false; }
};


/*!
\brief Templatized float initialization values.
*/

template<>
mClass(Basic) Init<float>
{
public:
    static float	val()		{ return 0.0f; }
};


/*!
\brief Templatized double initialization values.
*/

template<>
mClass(Basic) Init<double>
{
public:
    static double	val()		{ return 0.0; }
};


/*!
\brief Templatized const char* initialization values.
*/

template<>
mClass(Basic) Init<const char*>
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
