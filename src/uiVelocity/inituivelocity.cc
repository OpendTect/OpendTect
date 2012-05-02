/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID mUnusedVar = "$Id: inituivelocity.cc,v 1.8 2012-05-02 15:12:25 cvskris Exp $";

#include "moddepmgr.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"


mDefModInitFn(uiVelocity)
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();
}
