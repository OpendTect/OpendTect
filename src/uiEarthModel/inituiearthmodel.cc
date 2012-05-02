/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituiearthmodel.cc,v 1.8 2012-05-02 15:12:04 cvskris Exp $";

#include "moddepmgr.h"
#include "uibodyposprovgroup.h"
#include "uisurfaceposprov.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );

    uiBodyPosProvGroup::initClass();
    uiSurfacePosProvGroup::initClass();
}
