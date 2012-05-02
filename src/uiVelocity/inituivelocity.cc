/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* mUnusedVar rcsID = "$Id: inituivelocity.cc,v 1.7 2012-05-02 11:54:01 cvskris Exp $";

#include "moddepmgr.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"


mDefModInitFn(uiVelocity)
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();
}
