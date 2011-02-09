#ifndef staticstring_h
#define staticstring_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: staticstring.h,v 1.3 2011-02-09 17:01:15 cvskarthika Exp $
________________________________________________________________________
-*/


#include "sets.h"
#include "thread.h"
#include "bufstringset.h"


/*!Class that keeps one static string per thread. This enables temporary passing
   of static strings where needed. */

mClass StaticStringManager
{
public:
    BufferString&		getString();
    static StaticStringManager&	STM();

    				~StaticStringManager();
protected:

    BufferStringSet     	strings_;
    ObjectSet<void>     	threadids_;
    Threads::Mutex		lock_;
};

#endif
