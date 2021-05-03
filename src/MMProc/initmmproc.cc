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
#include "procdescdata.h"

mDefModInitFn(MMProc)
{
    mIfNotFirstTime( return );

    Batch::SingleJobDispatcher::initClass();
    Batch::MMJobDispatcher::initClass();
    Batch::ClusterJobDispatcher::initClass();
#ifdef  __win__
    ePDD().add( "od_remoteservice",
       Batch::MMProgDef::sMMProcDesc(), ProcDesc::DataEntry::ODv6 );
#endif //  __win__
}
