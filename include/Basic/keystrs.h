#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Mar 2002
 RCS:		$Id: keystrs.h,v 1.1 2002-03-26 17:02:30 bert Exp $
________________________________________________________________________

-*/
 
 
#include <gendefs.h>

#undef mImpl

#ifndef KEYSTRS_IMPL
# define mImpl(s) = s
#else
# define mImpl(s) /* empty */
#endif

namespace sKey
{

    const char*		Yes		mImpl("Yes");
    const char*		No		mImpl("No");
    const char*		Type		mImpl("Type");
    const char*		Undef		mImpl("Undefined");
    const char*		Factor		mImpl("Factor");
    const char*		Name		mImpl("Name");
    const char*		Title		mImpl("Title");
    const char*		Value		mImpl("Value");
    const char*		FileName	mImpl("File name");
    const char*		IOSelection	mImpl("I/O Selection");

    const char*		Minimum		mImpl("Minimum");
    const char*		Maximum		mImpl("Maximum");
    const char*		Average		mImpl("Average");
    const char*		Median		mImpl("Median");
    const char*		Sum		mImpl("Sum");
    const char*		StdDev		mImpl("StdDev");

};


#undef mImpl

#endif
