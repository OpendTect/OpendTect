/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2016
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "systeminfo.h"

typedef const char* (*constcharFromBoolFn)(bool);
mGlobal(Basic) void setGlobal_Basic_Fns(constcharFromBoolFn);

void NetworkHttpFileSystemAccessinitClass();

mDefModInitFn(Network)
{
    mIfNotFirstTime( return );

    NetworkHttpFileSystemAccessinitClass();
    setGlobal_Basic_Fns( System::localAddress );
}
