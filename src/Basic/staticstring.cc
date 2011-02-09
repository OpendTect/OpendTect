/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID = "$Id: staticstring.cc,v 1.3 2011-02-09 17:01:15 cvskarthika Exp $";

#include "staticstring.h"

#include "keystrs.h"

BufferString& StaticStringManager::getString()
{
    void* threadid = Threads::Thread::currentThread();
    Threads::MutexLocker lock( lock_ );
    int idx = threadids_.indexOf( threadid );
    if ( idx<0 )
    {
	idx = threadids_.size();
	threadids_ += threadid;
	strings_.add( sKey::EmptyString );
    }

    return *strings_[idx];
}


StaticStringManager::~StaticStringManager()
{ }


StaticStringManager& StaticStringManager::STM()
{
    static StaticStringManager stm;
    return stm;
}
