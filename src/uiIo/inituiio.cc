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
#include "uimsg.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "mmbatchjobdispatch.h"


class uiMMBatchJobDispatcherLauncher : public uiBatchJobDispatcherLauncher
{
public:

uiMMBatchJobDispatcherLauncher( Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js) {}

mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			uiMMBatchJobDispatcherLauncher,
			Batch::JobSpec&,"Multi-Machine","Multi-Job/Machine");

    virtual Batch::JobDispatcher&	gtDsptchr() { return jd_; }
    Batch::MMJobDispatcher		jd_;

};



mDefModInitFn(uiIo)
{
    mIfNotFirstTime( return );

    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();

    uiMMBatchJobDispatcherLauncher::initClass();

    uiProcSettings::initClass();
}
