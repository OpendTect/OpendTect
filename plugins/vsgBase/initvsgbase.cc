/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "moddepmgr.h"
#include "visvsgdataman.h"


mDefModInitFn(vsgBase)
{
    mIfNotFirstTime( return );
    ODVsg::initStdClasses();
}
