/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cmddrivermod.h"

#include "uimenu.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uicmddrivermgr.h"
#include "uistrings.h"
#include "odplugin.h"


namespace CmdDrive
{

mDefODPluginInfo(CmdDriver)
{
    static PluginInfo retpi(
	"Command driver (GUI)",
	"Used for testing and general 'scripting'." );
    return &retpi;
}


class CmdDriverPIMgr : public uiPluginInitMgr
{
mODTextTranslationClass(CmdDriverPIMgr)
public:

CmdDriverPIMgr()
{
    init();
}


~CmdDriverPIMgr()
{
}


void init() override
{
    if ( !ODMainWin() )
	return;

    mgr_ = &uiCmdDriverMgr::getMgr( true );
}


void dTectMenuChanged() override
{
    if ( !ODMainWin() )
	return;

    auto* cmdmnuitm = new uiAction( m3Dots( uiCmdDriverMgr::usrDispNm() ),
				    mCB(mgr_,uiCmdDriverMgr,showDlgCB) );
    appl_.menuMgr().toolsMnu()->insertAction( cmdmnuitm );

    auto* mnuitm = new uiAction(
		m3Dots(toUiString("Command Driver Script Runner")),
		mCB(mgr_,uiCmdDriverMgr,showScriptRunnerCB) );
    appl_.menuMgr().toolsMnu()->insertAction( mnuitm );
}


void cleanup() override
{
    mgr_->cleanup();
}

protected:

    uiCmdDriverMgr* mgr_	= nullptr;
};


mDefODInitPlugin(CmdDriver)
{
    mDefineStaticLocalObject( PtrMan<CmdDriverPIMgr>, theinst_,
				= new CmdDriverPIMgr() );
    return nullptr;
}

} // namespace CmdDrive
