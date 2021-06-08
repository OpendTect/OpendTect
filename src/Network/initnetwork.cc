/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Jun 2021
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "systeminfo.h"

extern "C" { mGlobal(Basic) const char* GetLocalAddress( bool ipv4only ); }

typedef const char* (*constcharFromBoolFn)(bool);
mGlobal(Basic) void setGlobal_Basic_Fns(constcharFromBoolFn);

mDefModInitFn(Network)
{
    mIfNotFirstTime( return );

    setGlobal_Basic_Fns( System::localAddress );
}
