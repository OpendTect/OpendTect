/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uistratbasiclayseqgendesc.h"

mDefModInitFn(uiStrat)
{
    mIfNotFirstTime( return );

    uiBasicLayerSequenceGenDesc::initClass();
}
