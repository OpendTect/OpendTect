#ifndef staticstring_h
#define staticstring_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: staticstring.h,v 1.1 2011-01-07 17:16:33 cvskris Exp $
________________________________________________________________________
-*/


#include "sets.h"
#include "thread.h"


/*!Class that keeps one static string per thread. This enables temporary passing
   of static strings where needed. */

mClass StaticStringManager
{
public:
    char*         		getString();
    static StaticStringManager&	STM();

    				~StaticStringManager();
protected:

    ObjectSet<char>     	strings_;
    ObjectSet<void>     	threadids_;
    Threads::Mutex		lock_;
};

#endif
