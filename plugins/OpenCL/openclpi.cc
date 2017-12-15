/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/

#include "odplugin.h"

#include "openclmod.h"
#include "openclplatform.h"


mDefODPluginEarlyLoad(OpenCL)
mDefODPluginInfo(OpenCL)
{
    mDefineStaticLocalObject( PluginInfo, retpi,
	( "OpenCL Attributes (Base)", "OpenCL Processing",
	mODPluginCreator, mODPluginVersion,
	"Adds OpenCL-based attributes" ) );
    return &retpi;
}


mDefODInitPlugin(OpenCL)
{
    OpenCL::Platform::initClass();

    return 0;
}
