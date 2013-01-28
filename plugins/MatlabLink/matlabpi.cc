/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "matlablinkmod.h"
#include "matlabstep.h"

mDefODPluginEarlyLoad(MatlabLink)
mDefODPluginInfo(MatlabLink)
{
    static PluginInfo retpi = {
	"MATLAB (base)",
	"dGB Earth Sciences",
	"=od",
    	"MATLAB - base" };
    return &retpi;
}


mDefODInitPlugin(MatlabLink)
{
    VolProc::MatlabStep::initClass();
    return 0;
}
