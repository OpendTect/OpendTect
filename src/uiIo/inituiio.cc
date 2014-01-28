/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "uit2dconvsel.h"
#include "uibatchjobdispatcherlauncher.h"
#include "mmbatchjobdispatch.h"
#include "uimsg.h"


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

    uiT2DLinConvSelGroup::initClass();

    uiMMBatchJobDispatcherLauncher::initClass();
}
