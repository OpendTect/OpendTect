/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Sep 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cmddriverpi.cc,v 1.32 2009-11-27 11:34:45 cvsnageswara Exp $";

#include "uimain.h"
#include "uiodmenumgr.h"
#include "cmddrivermgr.h"
#include "plugins.h"

namespace CmdDrive 
{


mExternC int GetCmdDriverPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetCmdDriverPluginInfo()
{
    static PluginInfo retpii = {
	"Command driver",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}


//static const char* autoexecfnm = "autoexec.cmd";


mExternC const char* InitCmdDriverPlugin( int, char** )
{
    static uiCmdDriverMgr* mgr = 0;
    if ( mgr ) return 0;
    mgr = new uiCmdDriverMgr( *ODMainWin() );
    return 0;
}


}; // namespace CmdDrive
