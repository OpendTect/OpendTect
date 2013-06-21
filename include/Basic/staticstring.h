#ifndef staticstring_h
#define staticstring_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id$
________________________________________________________________________
-*/


#include "basicmod.h"
#include "sets.h"
#include "threadlock.h"
#include "bufstringset.h"


/*!
\brief Class that keeps one static string per thread. This enables temporary
passing of static strings where needed.
*/

mExpClass(Basic) StaticStringManager
{
public:
    BufferString&		getString();

    				~StaticStringManager();
protected:

    BufferStringSet     	strings_;
    ObjectSet<const void>     	threadids_;
    Threads::Lock		lock_;
};


#endif

