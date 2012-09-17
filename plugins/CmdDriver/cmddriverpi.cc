/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: cmddriverpi.cc,v 1.38 2012-09-17 12:49:54 cvsjaap Exp $";

#include "uimenu.h"
#include "uimain.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uicmddrivermgr.h"
#include "odplugin.h"


mDefODPluginInfo(CmdDriver)
{
    static PluginInfo retpii = {
	"Command driver",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}


mDefODInitPlugin(CmdDriver)
{
    static CmdDrive::uiCmdDriverMgr* mgr = 0;
    if ( mgr ) return 0;
    mgr = new CmdDrive::uiCmdDriverMgr( *ODMainWin() );

    static uiMenuItem* cmdmnuitm = 0;
    if ( cmdmnuitm ) return 0;
    cmdmnuitm = new uiMenuItem( "Command &Driver ..." );

    ODMainWin()->menuMgr().toolsMnu()->insertItem( cmdmnuitm );
    cmdmnuitm->activated.notify( mCB(mgr,CmdDrive::uiCmdDriverMgr,showDlgCB) );

    return 0;
}
