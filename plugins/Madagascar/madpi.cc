/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "maddefs.h"
#include "odplugin.h"
#include "madprocflowtr.h"

mDefODPluginEarlyLoad(Madagascar)
mDefODPluginInfo(Madagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar (base)",
	"dGB - Bert Bril",
	"=od",
	"The Madagascar batch-level tools.") );
    return &retpi;
}


mDefODInitPlugin(Madagascar)
{
    ODMadProcFlowTranslatorGroup::initClass();
    dgbODMadProcFlowTranslator::initClass();

    mDefineStaticLocalObject( BufferString, prescanmsg, =ODMad::PI().errMsg());
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
