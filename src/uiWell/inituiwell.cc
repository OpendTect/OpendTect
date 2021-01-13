/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
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
}
