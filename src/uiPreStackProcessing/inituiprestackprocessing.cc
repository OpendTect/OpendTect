/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
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
