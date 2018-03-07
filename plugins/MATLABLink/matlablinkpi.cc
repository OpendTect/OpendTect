/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

#include "matlablinkmod.h"
#include "odplugin.h"
#include "matlabstep.h"

mDefODPluginEarlyLoad(MATLABLink)
mDefODPluginInfo(MATLABLink)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"MATLAB Link (Base)",
	mMATLABLinkPackage,
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpi;
}


mDefODInitPlugin(MATLABLink)
{
    VolProc::MatlabStep::initClass();
    return 0;
}
