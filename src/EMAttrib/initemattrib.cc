/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "horizonattrib.h"

mDefModInitFn(EMAttrib)
{
    mIfNotFirstTime( return );

    Attrib::Horizon::initClass();
}
