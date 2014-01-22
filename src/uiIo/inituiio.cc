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
bool isSuitedFor( const char* pnm ) const { return jd_.isSuitedFor(pnm); }
bool canHandle( const Batch::JobSpec& js ) const { return jd_.canHandle(js); }
const char* getInfo() const { return jd_.description(); }
bool go( uiParent* p )
{
    if ( !jd_.go(jobspec_) )
    {
	const char* errmsg = jd_.errMsg();
	uiMSG().error( errmsg ? errmsg
			      : "Cannot Muti-Machine processing program" );
	return false;
    }
    return true;
}
mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			    uiMMBatchJobDispatcherLauncher,
			    Batch::JobSpec&,"Multi-Machine","Multi-Machine");

    Batch::MMJobDispatcher jd_;

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
