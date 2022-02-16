/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
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
#ifdef __win__
    ePDD().add( "od_SeisMMBatch",
	Batch::SeisMMProgDef::sSeisMMProcDesc(), ProcDesc::DataEntry::ODv6 );
#endif // __win__
}
