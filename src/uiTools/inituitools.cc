/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uiarray2dinterpol.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uifontsel.h"
#include "uicrssystem.h"
#include "uigridder2d.h"
#include "uiinterpollayermodel.h"
#include "uiraytrace1d.h"
#include "uisettings.h"

mDefModInitFn(uiTools)
{
    mIfNotFirstTime( return );

    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
    uiTriangulationArray2DInterpol::initClass();
    uiExtensionArray2DInterpol::initClass();
    uiVrmsRayTracer1D::initClass();

    uiSingleBatchJobDispatcherLauncher::initClass();
    uiZSliceInterpolationModel::initClass();

    uiGeneralSettingsGroup::initClass();
    uiVisSettingsGroup::initClass();
    uiFontSettingsGroup::initClass();
    Coords::uiProjectionBasedSystem::initClass();

    uiSettsMgr();
}
