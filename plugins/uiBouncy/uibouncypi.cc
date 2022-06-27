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
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Bouncy thingy (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Karthika)",
	"=od",
	"User Interface for Bouncy plugin" ))
    return &retpi;
}


mDefODInitPlugin(uiBouncy)
{
    mDefineStaticLocalObject( PtrMan<uiBouncyMgr>, theinst_,
	   = new uiBouncy::uiBouncyMgr( ODMainWin() ) );
    if ( !theinst_ )
	return "Cannot instantiate the uiBouncy plugin";

    return nullptr;
}
