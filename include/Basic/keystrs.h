#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Mar 2002
 RCS:		$Id: keystrs.h,v 1.2 2002-03-27 08:18:09 bert Exp $
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

    extern const char*	Yes		mImpl("Yes");
    extern const char*	No		mImpl("No");
    extern const char*	Type		mImpl("Type");
    extern const char*	Undef		mImpl("Undefined");
    extern const char*	Factor		mImpl("Factor");
    extern const char*	Name		mImpl("Name");
    extern const char*	Title		mImpl("Title");
    extern const char*	Value		mImpl("Value");
    extern const char*	FileName	mImpl("File name");
    extern const char*	IOSelection	mImpl("I/O Selection");

    extern const char*	Minimum		mImpl("Minimum");
    extern const char*	Maximum		mImpl("Maximum");
    extern const char*	Average		mImpl("Average");
    extern const char*	Median		mImpl("Median");
    extern const char*	Sum		mImpl("Sum");
    extern const char*	StdDev		mImpl("StdDev");

};


#undef mImpl

#endif
