/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "clusterjobdispatch.h"
#include "mmbatchjobdispatch.h"

mDefModInitFn(MMProc)
{
    mIfNotFirstTime( return );

    Batch::SingleJobDispatcher::initClass();
    Batch::MMJobDispatcher::initClass();
    Batch::ClusterJobDispatcher::initClass();
}
