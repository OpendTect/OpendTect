/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : February 2013
-*/


#include "uimatlablinkmod.h"

#include "uimatlabstep.h"
#include "odplugin.h"


mDefODPluginInfo(uiMATLABLink)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"MATLAB link",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"A link to MATLAB."
	    "\nThis is the User interface of the link." ));
    return &retpi;
}


mDefODInitPlugin(uiMATLABLink)
{
    VolProc::uiMatlabStep::initClass();

    // Add custom dir
    return 0;
}
