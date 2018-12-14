/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uiveldesc.h"
#include "uiseismmjobdispatch.h"
#include "uiseiswriteopts.h"
#include "uiseisposprovgroup.h"


mDefModInitFn(uiSeis)
{
    mIfNotFirstTime( return );

    uiTime2Depth::initClass();
    uiDepth2Time::initClass();

    Batch::MMJobDispatcher::addDef( new Batch::SeisMMProgDef );

    uiSeisBlocksOpts::initClass();
    uiCBVSVolOpts::initClass();
    uiCBVSPS3DOpts::initClass();

    uiSeisPosProvGroup::initClass();
}
