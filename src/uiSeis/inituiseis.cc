/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uiseismmjobdispatch.h"
#include "uiseiswriteopts.h"
#include "uiseisposprovgroup.h"
#include "uisynthgrp.h"
#include "uiveldesc.h"

#include "procdescdata.h"

mDefModInitFn(uiSeis)
{
    mIfNotFirstTime( return );

    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
    uiBaseSynthSeis::initClass();

    Batch::MMJobDispatcher::addDef( new Batch::SeisMMProgDef );

    uiCBVSVolOpts::initClass();
    uiCBVSPS3DOpts::initClass();

    uiSeisPosProvGroup::initClass();
}
