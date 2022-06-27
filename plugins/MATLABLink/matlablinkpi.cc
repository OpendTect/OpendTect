/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
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
