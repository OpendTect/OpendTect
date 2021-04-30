/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/


#include "uiodmain.h"
#include "odplugin.h"
#include "uibouncymgr.h"


mDefODPluginInfo(uiBouncy)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Bouncy thingy",
	"OpendTect",
	"dGB (Karthika)",
	"4.0",
	"Having some fun in OpendTect.") );
    return &retpi;
}


mDefODInitPlugin(uiBouncy)
{
    mDefineStaticLocalObject( PtrMan<uiBouncyMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiBouncy::uiBouncyMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate Bouncy plugin";

    return 0;
