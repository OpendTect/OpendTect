/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "ceemdattribmod.h"
#include "ceemdattrib.h"

mDefODPluginEarlyLoad(CEEMDAttrib)
mDefODPluginInfo(CEEMDAttrib)
{
    static PluginInfo retpi(
	"CEEMD (Base)",
	"CEEMD - Complete Ensemble Empirical Mode Decomposition Attribute" );
    return &retpi;
}


mDefODInitPlugin(CEEMDAttrib)
{
    Attrib::CEEMD::initClass();
    return nullptr;
}
