/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "welllogattrib.h"

mDefModInitFn(WellAttrib)
{
    mIfNotFirstTime( return );

    Attrib::WellLog::initClass();
}
