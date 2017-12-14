/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "matlablinkmod.h"
#include "matlabstep.h"

mDefODPluginEarlyLoad(MATLABLink)
mDefODPluginInfo(MATLABLink)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"MATLAB (base)",
	"MatLab Link",
	"opendtect.org",
	"1.0",
	"MATLAB - base" ) );
    retpi.useronoffselectable_ = true;
    return &retpi;
}


mDefODInitPlugin(MATLABLink)
{
    VolProc::MatlabStep::initClass();
    return 0;
}
