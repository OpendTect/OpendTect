/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/


#include "moddepmgr.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"
#include "uilinearveltrans.h"


mDefModInitFn(uiVelocity)
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();

    Vel::uiLinearVelTransform::initClass();
}
