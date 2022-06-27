/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/


#include "odplugin.h"

#include "ceemdattribmod.h"
#include "ceemdattrib.h"

mDefODPluginEarlyLoad(CEEMDAttrib)
mDefODPluginInfo(CEEMDAttrib)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"CEEMD (Base)",
	"OpendTect",
	"dGB Earth Sciences (Paul de Groot)",
	"=od",
	"CEEMD - Complete Ensemble Empirical Mode Decomposition Attribute" ))
    return &retpi;
}


mDefODInitPlugin(CEEMDAttrib)
{
    Attrib::CEEMD::initClass();
    return nullptr;
}
