/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uiprestackagc.h"
#include "uiprestackmute.h"
#include "uiprestacklateralstack.h"
#include "uiprestackanglemute.h"
#include "uiprestackmmjobdispatch.h"
#include "uiprestacktrimstatics.h"

mDefModInitFn(uiPreStackProcessing)
{
    mIfNotFirstTime( return );

    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
    PreStack::uiLateralStack::initClass();
    PreStack::uiAngleMute::initClass();
#ifdef __debug__
    PreStack::uiTrimStatics::initClass();
#endif

    Batch::MMJobDispatcher::addDef( new Batch::PreStackMMProgDef );
}
