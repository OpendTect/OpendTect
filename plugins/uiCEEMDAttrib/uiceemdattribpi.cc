/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/

#include "uiceemdattribmod.h"
#include "uiceemdattrib.h"
#include "odplugin.h"
#include <iostream>


mDefODPluginInfo(uiCEEMDAttrib)
{
    mDefineStaticLocalObject(PluginInfo, retpi, (
	"CEEMD (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Paul de Groot)",
	"=od",
	"User Interface for Complete Ensemble Empirical Mode Decomposition "
	"attribute" ))
    return &retpi;
}


mDefODInitPlugin(uiCEEMDAttrib)
{
    uiCEEMDAttrib::initClass();
    return nullptr;
}
