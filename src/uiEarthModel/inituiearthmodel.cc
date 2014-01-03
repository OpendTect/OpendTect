/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uibodyposprovgroup.h"
#include "uihorinterpol.h"
#include "uisurfaceposprov.h"
#include "uiimphorizon.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );
    
    uiImportHorizon::initClass();
    uiInvDistHor3DInterpol::initClass();
    uiTriangulationHor3DInterpol::initClass();
    uiExtensionHor3DInterpol::initClass();

    uiBodyPosProvGroup::initClass();
    uiSurfacePosProvGroup::initClass();
}
