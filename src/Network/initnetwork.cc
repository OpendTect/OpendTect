/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          Jun 2021
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

    NetworkHttpFileSystemAccessinitClass();
    setGlobal_Basic_Fns( System::localAddress );
}
