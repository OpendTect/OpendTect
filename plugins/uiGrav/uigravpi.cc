/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigravhorcalc.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "vishorizondisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "odplugin.h"

#include "uigravmod.h"


mDefODPluginInfo(uiGrav)
{
    static PluginInfo retpi(
	"Gravity calculation (GUI)",
	"Calculates gravity between horizons" );
    return &retpi;
}


class uiGravMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiGravMgr);
public:

			uiGravMgr();

private:

    void		doDlg(CallBacker*);

    uiVisMenuItemHandler mnuitemhndlr_;
};


uiGravMgr::uiGravMgr()
    : uiPluginInitMgr()
    , mnuitemhndlr_(visSurvey::HorizonDisplay::sFactoryKeyword(),
		    appl().applMgr().visServer(),"Calculate Gravity",
		    mCB(this,uiGravMgr,doDlg))
{
    init();
}

void uiGravMgr::doDlg( CallBacker* )
{
    const int displayid = mnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv->getObject(displayid));
    if ( !hd ) return;

    uiGravHorCalc dlg( &appl(), hd->getObjectID() );
    dlg.go();
}


mDefODInitPlugin(uiGrav)
{
    mDefineStaticLocalObject( PtrMan<uiGravMgr>, theinst_,
				= new uiGravMgr() );
    if ( !theinst_ )
	return "Cannot instantiate Gravity plugin";

    return nullptr;
}
