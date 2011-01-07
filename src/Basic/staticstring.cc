/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID = "$Id: staticstring.cc,v 1.1 2011-01-07 17:16:33 cvskris Exp $";

#include "staticstring.h"

char* StaticStringManager::getString()
{
    void* threadid = Threads::Thread::currentThread();
    Threads::MutexLocker lock( lock_ );
    int idx = threadids_.indexOf( threadid );
    if ( idx<0 )
    {
	idx = threadids_.size();
	threadids_ += threadid;
	strings_ += new char[255];
    }

    return strings_[idx];
}


StaticStringManager::~StaticStringManager()
{
    deepEraseArr( strings_ );
}


StaticStringManager& StaticStringManager::STM()
{
    static StaticStringManager stm;
    return stm;
}
