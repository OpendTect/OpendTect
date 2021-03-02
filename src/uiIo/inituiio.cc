/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uibatchlaunch.h"
#include "uiclusterjobprov.h"
#include "uilatlong2coord.h"
#include "uimsg.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "uisimpletimedepthmodel.h"
#include "uisurvinfoed.h"
#include "uisurvey.h"
#include "ui2dsip.h"
#include "envvars.h"
#include "mmbatchjobdispatch.h"
#include "settings.h"


static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";


class uiMMBatchJobDispatcherLauncher : public uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiMMBatchJobDispatcherLauncher)
public:

uiMMBatchJobDispatcherLauncher( Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js) {}

mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			uiMMBatchJobDispatcherLauncher,
			Batch::JobSpec&,"Multi-Machine",
			tr("Distributed Computing"));

    virtual Batch::JobDispatcher&	gtDsptchr() { return jd_; }
    Batch::MMJobDispatcher		jd_;

};


static bool enabClusterProc()
{
    bool enabclusterproc = false;
    const bool hassetting =
	Settings::common().getYN( sKeyClusterProc, enabclusterproc );
    if ( !hassetting )
	enabclusterproc = GetEnvVarYN( sKeyClusterProcEnv );
    return enabclusterproc;
}


mDefModInitFn(uiIo)
{
    mIfNotFirstTime( return );

    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();

    uiMMBatchJobDispatcherLauncher::initClass();
    Batch::ClusterJobDispatcher::addDef( new Batch::SimpleClusterProgDef );
    if ( enabClusterProc() )
	uiClusterJobDispatcherLauncher::initClass();

    uiProcSettings::initClass();
    Coords::uiUnlocatedXYSystem::initClass();
    Coords::uiAnchorBasedXYSystem::initClass();

    uiSimpleTimeDepthTransform::initClass();

    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );
    uiSurveyInfoEditor::addInfoProvider(new uiNavSurvInfoProvider);
    uiSurveyInfoEditor::addInfoProvider( new uiCopySurveySIP );
    uiSurveyInfoEditor::addInfoProvider( new uiSurveyFileSIP );
}
