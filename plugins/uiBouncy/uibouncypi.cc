/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiodmain.h"
#include "odplugin.h"
#include "uibouncymgr.h"


mDefODPluginInfo(uiBouncy)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Bouncy thingy", mODPluginGamesPackage,
	mODPluginCreator, mODPluginVersion,
	"Shoot up the horizons but beware of the nasty moving wells!" ) );
    return &retpi;
}


mDefODInitPlugin(uiBouncy)
{
    mDefineStaticLocalObject( PtrMan<uiBouncyMgr>, theinst_, = 0 );
    if ( theinst_ )
	return 0;

    theinst_ = new uiBouncy::uiBouncyMgr( ODMainWin() );
    return 0;
}
