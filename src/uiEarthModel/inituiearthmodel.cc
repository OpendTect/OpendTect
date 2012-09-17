/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiearthmodel.cc,v 1.6 2011/11/28 18:48:46 cvsyuancheng Exp $";

#include "moddepmgr.h"
#include "uibodyposprovgroup.h"
#include "uisurfaceposprov.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );

    uiBodyPosProvGroup::initClass();
    uiSurfacePosProvGroup::initClass();
}
