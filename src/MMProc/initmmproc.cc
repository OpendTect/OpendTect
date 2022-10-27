/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
       Batch::MMProgDef::sMMProcDesc(), ProcDesc::DataEntry::OD );
#endif //  __win__
}
