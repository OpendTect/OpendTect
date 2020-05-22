/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Jan 2020
-*/

#include "odplugin.h"
#include "uimsg.h"
#include "uicrashmemod.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"

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


class uiCrashMgr :  public CallBacker
{ mODTextTranslationClass(uiCrashMgr);
public:

			uiCrashMgr(uiODMain&);

    uiODMain&		appl;
    void		crashCB(CallBacker*);
    void		doCrash();
};


uiCrashMgr::uiCrashMgr( uiODMain& a )
	: appl(a)
{
    uiAction* newitem = new uiAction( toUiString("Force Crash!"),
					  mCB(this,uiCrashMgr,crashCB) );
    appl.menuMgr().utilMnu()->insertAction( newitem );
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
    mDefineStaticLocalObject( PtrMan<uiCrashMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiCrashMgr( *ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate The Crash plugin";

    return 0; // All OK - no error messages
}

