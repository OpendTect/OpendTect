/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uistratbasiclayseqgendesc.h"

mDefModInitFn(uiStrat)
{
    mIfNotFirstTime( return );

    uiBasicLayerSequenceGenDesc::initClass();
}
