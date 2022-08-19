/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uiodstratlayermodelmgr.h"
#include "uiwelllogattrib.h"

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    uiStratLayerModelManager::initClass();
    uiWellLogAttrib::initClass();
}
