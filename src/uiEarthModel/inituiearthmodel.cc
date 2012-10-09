/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "moddepmgr.h"
#include "uibodyposprovgroup.h"
#include "uisurfaceposprov.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );

    uiBodyPosProvGroup::initClass();
    uiSurfacePosProvGroup::initClass();
}
