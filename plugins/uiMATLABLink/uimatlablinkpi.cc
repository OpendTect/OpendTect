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
	"MATLAB Link", mMATLABLinkPackage,
	mODPluginCreator, mODPluginVersion,
	"Link to MATLAB services" ));
    retpi.useronoffselectable_ = true;
    return &retpi;
}


mDefODInitPlugin(uiMATLABLink)
{
    VolProc::uiMatlabStep::initClass();
    return 0;
}
