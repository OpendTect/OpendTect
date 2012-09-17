/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.6 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uivelocityfunctionvolume.h"
#include "uivelocityfunctionstored.h"


mDefModInitFn(uiVelocity)
{
    mIfNotFirstTime( return );

    Vel::uiVolumeFunction::initClass();
    Vel::uiStoredFunction::initClass();
}
