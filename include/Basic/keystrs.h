#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2002
 RCS:		$Id: keystrs.h,v 1.5 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

-*/
 
 
#include <gendefs.h>

#undef mImpl

#ifdef KEYSTRS_IMPL
# define mImpl(s) = s
#else
# define mImpl(s) /* empty */
#endif

/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    extern const char*	Color		mImpl("Color");
    extern const char*	Depth		mImpl("Depth");
    extern const char*	Desc		mImpl("Description");
    extern const char*	Factor		mImpl("Factor");
    extern const char*	FileName	mImpl("File name");
    extern const char*	IOSelection	mImpl("I/O Selection");
    extern const char*	Name		mImpl("Name");
    extern const char*	No		mImpl("No");
    extern const char*	Pars		mImpl("Parameters");
    extern const char*	Type		mImpl("Type");
    extern const char*	Title		mImpl("Title");
    extern const char*	Undef		mImpl("Undefined");
    extern const char*	Value		mImpl("Value");
    extern const char*	Yes		mImpl("Yes");

    extern const char*	Average		mImpl("Average");
    extern const char*	Maximum		mImpl("Maximum");
    extern const char*	Median		mImpl("Median");
    extern const char*	Minimum		mImpl("Minimum");
    extern const char*	StdDev		mImpl("StdDev");
    extern const char*	Sum		mImpl("Sum");

};


#undef mImpl

#endif
