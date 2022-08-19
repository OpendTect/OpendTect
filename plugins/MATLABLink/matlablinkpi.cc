/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"
#include "matlablinkmod.h"
#include "matlabstep.h"

mDefODPluginEarlyLoad(MATLABLink)
mDefODPluginInfo(MATLABLink)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"MATLAB (Base)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"MATLAB" ))
    return &retpi;
}


mDefODInitPlugin(MATLABLink)
{
    VolProc::MatlabStep::initClass();
    return nullptr;
}
