/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchjobdispatch.h"

mImplFactory(Batch::JobDispatcher,Batch::JobDispatcher::factory)


bool Batch::JobDispatcher::go( const Batch::JobSpec& js )
{
    BufferString reason;
    if ( !isSuitedFor(js,&reason) )
	{ errmsg_.set( "Cannot launch job:\n" ).add( reason ); return false; }

    jobspec_ = js;
    return launch();
}
