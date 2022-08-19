/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uivisemobj.h"

mDefModInitFn(uiVis)
{
    mIfNotFirstTime( return );
    
    uiHorizonSettings::initClass();
}
