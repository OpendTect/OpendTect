/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : February 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimatlablinkmod.h"

#include "uimatlabstep.h"
#include "odplugin.h"


mDefODPluginInfo(uiMatlabLink)
{
    static PluginInfo retpi = {
	"MATLAB link",
	"dGB Earth Sciences",
	"=od",
    	"A link to MATLAB."
	    "\nThis is the User interface of the link." };
    return &retpi;
}


mDefODInitPlugin(uiMatlabLink)
{
    VolProc::uiMatlabStep::initClass();

    // Add custom dir
    return 0;
}
