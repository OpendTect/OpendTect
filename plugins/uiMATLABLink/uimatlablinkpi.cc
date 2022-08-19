/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimatlablinkmod.h"

#include "uimatlabstep.h"
#include "odplugin.h"


mDefODPluginInfo(uiMATLABLink)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"MATLAB link (GUI)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"A link to MATLAB."
	    "\nThis is the User interface of the link." ))
    return &retpi;
}


mDefODInitPlugin(uiMATLABLink)
{
    VolProc::uiMatlabStep::initClass();

    // Add custom dir
    return nullptr;
}
