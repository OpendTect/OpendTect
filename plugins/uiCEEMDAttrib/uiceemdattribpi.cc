/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
