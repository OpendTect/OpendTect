/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "odruncontext.h"
#include "systeminfo.h"

using constcharFromBoolFn = const char*(*)(bool);
mGlobal(Basic) void setGlobal_Basic_Fns(constcharFromBoolFn);

void NetworkHttpFileSystemAccessinitClass();

mDefModInitFn(Network)
{
    mIfNotFirstTime(return);

#ifdef __debug__
    NetworkHttpFileSystemAccessinitClass();
#else
    const OD::RunCtxt ctxt = OD::RunCtxt::GetRunContext();
    if ( ctxt == OD::RunCtxt::TestProgCtxt )
	NetworkHttpFileSystemAccessinitClass();
    //TODO: remove when enabling release mode
#endif

    setGlobal_Basic_Fns( System::localAddress );
}
