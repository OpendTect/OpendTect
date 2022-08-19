/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "uicrashmemod.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uistrings.h"

mDefODPluginInfo(uiCrashMe)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"The Crash Plugin (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Raman)",
	"=od",
	"This is meant for developers to force a crash.\n"
	"Yeah, seriously! All it does is CRASH!") );
    return &retpi;
}


class uiCrashMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiCrashMgr);
public:

			uiCrashMgr();

private:

    void		dTectMenuChanged() override;

    void		crashCB(CallBacker*);
    void		doCrash();
};


uiCrashMgr::uiCrashMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiCrashMgr::dTectMenuChanged()
{
    appl().menuMgr().utilMnu()->insertAction(
			new uiAction( m3Dots(tr("Force Crash!")),
				      mCB(this,uiCrashMgr,crashCB) ) );
}


void uiCrashMgr::crashCB( CallBacker* )
{
    if ( uiMSG().askGoOn(toUiString("Do you want to CRASH OpendTect?")) )
	doCrash();
}


void uiCrashMgr::doCrash()
{
    DBG::forceCrash( false );
}


mDefODInitPlugin(uiCrashMe)
{
    mDefineStaticLocalObject( PtrMan<uiCrashMgr>, theinst_, = new uiCrashMgr());
    if ( !theinst_ )
	return "Cannot instantiate the Crash plugin";

    return nullptr;
}
