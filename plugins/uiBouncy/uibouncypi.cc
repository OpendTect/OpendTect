/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

static const char* rcsID = "$Id: uibouncypi.cc,v 1.2 2009-09-24 10:42:40 cvsnanne Exp $";

#include "uiodmain.h"
#include "plugins.h"
#include "uibouncymgr.h"


mExternC int GetuiBouncyPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiBouncyPluginInfo()
{
    static PluginInfo retpi = {
	"Bouncy thingy",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mExternC const char* InituiBouncyPlugin( int, char** )
{
    static uiBouncy::uiBouncyMgr* mgr = 0; 
    if ( mgr ) return 0;
    mgr = new uiBouncy::uiBouncyMgr( ODMainWin() );
    return 0;
}
