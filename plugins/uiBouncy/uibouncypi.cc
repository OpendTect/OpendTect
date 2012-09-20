/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uiodmain.h"
#include "odplugin.h"
#include "uibouncymgr.h"


mDefODPluginInfo(uiBouncy)
{
    static PluginInfo retpi = {
	"Bouncy thingy",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


mDefODInitPlugin(uiBouncy)
{
    static uiBouncy::uiBouncyMgr* mgr = 0; 
    if ( mgr ) return 0;
    mgr = new uiBouncy::uiBouncyMgr( ODMainWin() );
    return 0;
}
