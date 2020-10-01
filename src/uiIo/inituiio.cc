/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "envvars.h"
#include "genc.h"
#include "mmbatchjobdispatch.h"
#include "oddirs.h"
#include "settings.h"
#include "uiautosavesettings.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uibatchlaunch.h"
#include "uiclusterjobprov.h"
#include "uimsg.h"
#include "plugins.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "uisipimpl.h"
#include "uisurvinfoed.h"


static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";


class uiMMBatchJobDispatcherLauncher : public uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiMMBatchJobDispatcherLauncher)
public:

uiMMBatchJobDispatcherLauncher( Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js) {}

mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
		       uiMMBatchJobDispatcherLauncher,
		       Batch::JobSpec&,"Distributed",tr("Distributed"));

    virtual Batch::JobDispatcher&	gtDsptchr() { return jd_; }
    virtual bool			go(uiParent*,Batch::ID* =nullptr);
    Batch::MMJobDispatcher		jd_;

};


bool uiMMBatchJobDispatcherLauncher::go( uiParent* p, Batch::ID* batchid )
{
    const HostDataList hdl( false );
    const HostData* localhost = hdl.find( BufferString(GetLocalHostName()) );
    if ( !localhost )
    {
	if ( hdl.isEmpty() )
	    return uiBatchJobDispatcherLauncher::go( p, batchid );

	gUiMsg(p).error( tr("Cannot find configuration for localhost") );
	return false;
    }

    const File::Path localbasedatadir( GetBaseDataDir() );
    if ( localbasedatadir != localhost->getDataRoot() )
    {
	gUiMsg(p).error( tr("Current Data Root: '%1'\ndoes not match path in "
			  "batch processing configuration file:\n'%2'")
			  .arg( localbasedatadir.fullPath() )
			  .arg( localhost->getDataRoot().fullPath() ) );
	return false;
    }

    return uiBatchJobDispatcherLauncher::go( p, batchid );
}


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

    uiProcSettingsGroup::initClass();
    uiAutoSaverSettingsGroup::initClass();

    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );
    uiSurveyInfoEditor::addInfoProvider( new uiCopySurveySIP );
    uiSurveyInfoEditor::addInfoProvider( new uiSurveyFileSIP );
}
