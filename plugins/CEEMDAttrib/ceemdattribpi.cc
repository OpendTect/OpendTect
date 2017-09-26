/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/


#include "ceemdattribmod.h"
#include "ceemdattrib.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(CEEMDAttrib)


mDefODPluginInfo(CEEMDAttrib)
{
    mDefineStaticLocalObject (PluginInfo, retpi, (
	"CEEMD - Complete Ensemble Empirical Mode Decomposition"
	" Attribute Plugin",
	"OpendTect",
	"Paul de Groot",
	"=od",
	"CEEMD Plugin" ));
    return &retpi;
}


mDefODInitPlugin(CEEMDAttrib)
{
    Attrib::CEEMD::initClass();

    return 0;
}
