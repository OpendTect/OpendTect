/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uilassip.h"
#include "uisurvinfoed.h"
#include "uiwellposprov.h"
#include "uiwellt2dconv.h"
#include "uiwellwriteopts.h"
#include "uiwellinserter.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiWellPosProvGroup::initClass();
    uiWellT2DTransform::initClass();

    uiODWellWriteOpts::initClass();
    uiWellInserter::initClass();

    uiSurveyInfoEditor::addInfoProvider(new uiLASSurvInfoProvider);
}
