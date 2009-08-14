/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

static const char* rcsID = "$Id: bouncypi.cc,v 1.2 2009-08-14 16:44:49 cvskarthika Exp $";

#include "uimsg.h"
#include "uiodmain.h"
#include "plugins.h"
#include "bouncymgr.h"


mExternC mGlobal int GetBouncyPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC mGlobal PluginInfo* GetBouncyPluginInfo()
{
    static PluginInfo retpi = {
	"Bouncy thingy",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mExternC mGlobal const char* InitBouncyPlugin( int, char** )
{
    static BouncyMgr* mgr = 0; 
    if ( mgr ) return 0;
    mgr = new BouncyMgr( ODMainWin() );
    return 0;
}
