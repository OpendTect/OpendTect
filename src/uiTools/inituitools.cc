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


using fromFromSdlUiParSdlPtrFn = bool(*)(SurveyDiskLocation&,uiParent*,
			    const SurveyDiskLocation*,uiDialog::DoneResult* );
static fromFromSdlUiParSdlPtrFn dosurvselfn_ = nullptr;

mGlobal(uiTools) void setGlobal_uiTools_SurvSelFns(fromFromSdlUiParSdlPtrFn);
void setGlobal_uiTools_SurvSelFns( fromFromSdlUiParSdlPtrFn dosurvselfn )
{
    dosurvselfn_ = dosurvselfn;
}

extern "C" {
    mGlobal(uiTools) bool doSurveySelectionDlg(SurveyDiskLocation&,uiParent*,
				     const SurveyDiskLocation*,
				     uiDialog::DoneResult*);
}

mExternC(uiTools) bool doSurveySelectionDlg( SurveyDiskLocation& newsdl,
				uiParent* p, const SurveyDiskLocation* cursdl,
				uiDialog::DoneResult* doneres )
{
    return dosurvselfn_ ? (*dosurvselfn_)(newsdl,p,cursdl,doneres) : false;
}



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
