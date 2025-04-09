/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uilassip.h"
#include "uimnemonicsettings.h"
#include "uiwellinserter.h"
#include "uiwellposprov.h"
#include "uiwellt2dconv.h"
#include "uiwellwriteopts.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiWellPosProvGroup::initClass();
    uiWellT2DTransform::initClass();
    uiWellInitSIP();

    uiODWellWriteOpts::initClass();
    uiWellInserter::initClass();

    uiMnemonicSettings::initClass();
}
