/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: cmddriverpi.cc,v 1.35 2012-05-02 11:52:43 cvskris Exp $";

#include "uimain.h"
#include "uiodmenumgr.h"
#include "cmddrivermgr.h"
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
    return 0;
}
