/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2014
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uivisemobj.h"

mDefModInitFn(uiVis)
{
    mIfNotFirstTime( return );
    
    uiHorizonSettings::initClass();
}
