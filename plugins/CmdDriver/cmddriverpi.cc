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


static void initExtraCommands()
{
}

static void initExtraFunctions()
{
}

static void initExtraComposers()
{
}


mDefODInitPlugin(CmdDriver)
{
    uiCmdDriverMgr& mgr = uiCmdDriverMgr::getMgr( true );
    auto* cmdmnuitm = new uiAction( m3Dots( uiCmdDriverMgr::usrDispNm() ),
				    mCB(&mgr,uiCmdDriverMgr,showDlgCB) );
    ODMainWin()->menuMgr().toolsMnu()->insertAction( cmdmnuitm );

    initExtraCommands();
    initExtraFunctions();
    initExtraComposers();

    return nullptr;
}

} // namespace CmdDrive
