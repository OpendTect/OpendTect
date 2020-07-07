/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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

    mIfNotFirstTime( return nullptr );

    mDefineStaticLocalObject( uiAction*, cmdaction, = 0 );
    if ( cmdaction )
	return 0;

    cmdaction = new uiAction( m3Dots(uiCmdDriverMgr::usrDispNm()),
				"commanddriver" );
    cmdaction->setShortcut( "Ctrl+R" );

    ODMainWin()->menuMgr().toolsMnu()->insertAction( cmdaction );
    cmdaction->triggered.notify( mCB(&mgr,uiCmdDriverMgr,showDlgCB) );

    initExtraCommands();
    initExtraFunctions();
    initExtraComposers();

    return nullptr;
}

}; // namespace CmdDrive
