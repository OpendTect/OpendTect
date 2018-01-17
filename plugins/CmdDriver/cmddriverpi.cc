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
	"Command Driver", "Command Driver",
	mODPluginCreator, mODPluginVersion,
	"Used for testing and general 'scripting'." ));
    retpi.useronoffselectable_ = true;
    mSetPackageDisplayName( retpi, uiCmdDriverMgr::usrDispNm() );
    retpi.uidispname_ = retpi.uipackagename_;
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
    mDefineStaticLocalObject( uiCmdDriverMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiCmdDriverMgr( true );

    mDefineStaticLocalObject( uiAction*, cmdaction, = 0 );
    if ( cmdaction )
	return 0;

    cmdaction = new uiAction( m3Dots(uiCmdDriverMgr::usrDispNm()),
				"commanddriver" );
    cmdaction->setShortcut( "Ctrl+R" );

    ODMainWin()->menuMgr().toolsMnu()->insertAction( cmdaction );
    cmdaction->triggered.notify( mCB(mgr,uiCmdDriverMgr,showDlgCB) );

    initExtraCommands();
    initExtraFunctions();
    initExtraComposers();

    return 0;
}

}; // namespace CmdDrive
