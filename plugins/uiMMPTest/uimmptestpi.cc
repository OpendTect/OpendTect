
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uiodapplmgr.h"
#include "uimsg.h"
#include "uimmptest.h"
#include "hostdata.h"
#include "odplugin.h"


mDefODPluginInfo(uiMMPTest)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Multi-machine Processing Diagnostics",
	mODPluginODPackage,
	mODPluginCreator, mODPluginVersion,
	"Diagnose issues related to Multi-machine Processing") );
    return &retpi;
}


class uiMMPTestMgr	: public CallBacker
{ mODTextTranslationClass(uiMMPTestMgr)
public:

				uiMMPTestMgr(uiODMain*);

    uiODMain*			appl_;

    void			updateMenu(CallBacker*);
    void			mnuCB(CallBacker*);
};


uiMMPTestMgr::uiMMPTestMgr( uiODMain* a )
    : appl_(a)
{
    appl_->menuMgr().addAction( appl_->menuMgr().mmProcMenu(),
	    tr("Diagnostics"), "diagnostics", mCB(this,uiMMPTestMgr,mnuCB) );
}


void uiMMPTestMgr::mnuCB( CallBacker* )
{
    HostDataList hdlist( false );
    if ( hdlist.size() < 2 )
    {
	const bool setupnow = gUiMsg().askGoOn(
		tr("No remote machines set up for Batch prcessing"),
		tr("Setup now"), uiStrings::sClose() );
	if ( !setupnow )
	    return;

	appl_->applMgr().setupBatchHosts();
	hdlist.refresh();
	if ( hdlist.size() < 2 )
	    return;
    }

    Batch::JobSpec js( "od_mmptestbatch" );
    Batch::MMJobDispatcher dispatcher;
    dispatcher.setJobName( "MMP_Diagnostics" );
    dispatcher.go( js );
}


mDefODInitPlugin(uiMMPTest)
{
    mDefineStaticLocalObject( uiMMPTestMgr*, mgr, = 0 );
    if ( mgr )
	return 0;

    mgr = new uiMMPTestMgr( ODMainWin() );
    Batch::MMJobDispatcher::addDef( new Batch::TestMMProgDef );
    return 0;
}
