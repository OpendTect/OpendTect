/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
________________________________________________________________________

-*/

#include "uimenu.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uicmddrivermgr.h"
#include "odplugin.h"
#include "cmddrivermod.h"
//#include "coincommands.h"


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
 //   WheelCmd::initClass();
    //GetWheelCmd::initClass();
}

static void initExtraFunctions()
{}

static void initExtraComposers()
{
}


mDefODInitPlugin(CmdDriver)
{
    mDefineStaticLocalObject( uiCmdDriverMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiCmdDriverMgr( true );

    mDefineStaticLocalObject( uiAction*, cmdmnuitm, = 0 );
    if ( cmdmnuitm ) return 0;
    cmdmnuitm = new uiAction( toUiString("Command &Driver ...") );

    ODMainWin()->menuMgr().toolsMnu()->insertItem( cmdmnuitm );
    cmdmnuitm->triggered.notify( mCB(mgr,uiCmdDriverMgr,showDlgCB) );

    initExtraCommands();
    initExtraFunctions();
    initExtraComposers();

    return 0;
}

}; // namespace CmdDrive
