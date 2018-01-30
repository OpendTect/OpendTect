/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/


#include "maddefs.h"
#include "odplugin.h"
#include "madprocflowtr.h"

mDefODPluginEarlyLoad(Madagascar)
mDefODPluginInfo(Madagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar (Base)",
	mMadagascarLinkPackage,
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpi;
}


mDefODInitPlugin(Madagascar)
{
    ODMadProcFlowTranslatorGroup::initClass();
    dgbODMadProcFlowTranslator::initClass();

    mDefineStaticLocalObject( BufferString, prescanmsg,
			     = toString( ODMad::PI().errMsg() ) );
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
