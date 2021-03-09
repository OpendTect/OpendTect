/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Jan 2020
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
	"The Crash Plugin",
	"OpendTect",
	"Raman",
	"1.1.1",
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
    if ( gUiMsg().askGoOn(toUiString("Do you want to CRASH OpendTect?")) )
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
	return "Cannot instantiate The Crash plugin";

    return nullptr;
}

