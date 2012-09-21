/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uimenu.h"
#include "uimain.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uicmddrivermgr.h"
#include "odplugin.h"
#include "coincommands.h"


namespace CmdDrive
{

mDefODPluginInfo(CmdDriver)
{
    static PluginInfo retpii = {
	"Command driver",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}


static void initExtraCommands()
{
    WheelCmd::initClass();
    GetWheelCmd::initClass();
}

static void initExtraFunctions()
{}

static void initExtraComposers()
{
    WheelCmdComposer::initClass();
}


mDefODInitPlugin(CmdDriver)
{
    static uiCmdDriverMgr* mgr = 0;
    if ( mgr ) return 0;
    mgr = new uiCmdDriverMgr( *ODMainWin() );

    static uiMenuItem* cmdmnuitm = 0;
    if ( cmdmnuitm ) return 0;
    cmdmnuitm = new uiMenuItem( "Command &Driver ..." );

    ODMainWin()->menuMgr().toolsMnu()->insertItem( cmdmnuitm );
    cmdmnuitm->activated.notify( mCB(mgr,uiCmdDriverMgr,showDlgCB) );

    initExtraCommands();
    initExtraFunctions();
    initExtraComposers();

    return 0;
}

}; // namespace CmdDrive
