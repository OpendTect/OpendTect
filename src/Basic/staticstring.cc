/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "staticstring.h"
#include "thread.h"

#include "keystrs.h"

BufferString& StaticStringManager::getString()
{
    const void* threadid = Threads::currentThread();
    Threads::Locker lock( lock_ );
    int idx = threadids_.indexOf( threadid );
    if ( idx<0 )
    {
	idx = threadids_.size();
	threadids_ += threadid;
	strings_.add( sKey::EmptyString() );
    }

    return *strings_[idx];
}


StaticStringManager::~StaticStringManager()
{
}
