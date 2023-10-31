/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "systeminfo.h"

using constcharFromBoolFn = const char*(*)(bool);
mGlobal(Basic) void setGlobal_Basic_Fns(constcharFromBoolFn);

void NetworkHttpFileSystemAccessinitClass();

mDefModInitFn(Network)
{
    mIfNotFirstTime(return);

#ifdef __debug__
    NetworkHttpFileSystemAccessinitClass();
#endif

    setGlobal_Basic_Fns( System::localAddress );
}
