/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "maddefs.h"
#include "odplugin.h"
#include "madprocflowtr.h"

mDefODPluginEarlyLoad(Madagascar)
mDefODPluginInfo(Madagascar)
{
    static PluginInfo retpi(
	"Madagascar Link (Base)",
	"The Madagascar batch-level tools." );
    return &retpi;
}


mDefODInitPlugin(Madagascar)
{
    ODMadProcFlowTranslatorGroup::initClass();
    dgbODMadProcFlowTranslator::initClass();

    mDefineStaticLocalObject(BufferString, prescanmsg,
			     = ODMad::PI().errMsg().getFullString());
    return prescanmsg.isEmpty() ? nullptr : prescanmsg.buf();
}
