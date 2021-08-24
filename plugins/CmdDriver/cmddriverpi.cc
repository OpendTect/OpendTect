/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
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
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Command driver",
	"OpendTect",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." ));
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
