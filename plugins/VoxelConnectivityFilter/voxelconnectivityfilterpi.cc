/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "voxelconnectivityfilter.h"

mDefODPluginEarlyLoad(VoxelConnectivityFilter)

mDefODPluginInfo(VoxelConnectivityFilter)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"VoxelConnectivityFilter plugin (base)",
	"OpendTect",
	"dGB",
	"=od",
   	"(c) dGB Beheer BV. Development funded by Tetrale Technologies."));
    return &retpi;
}


mDefODInitPlugin(VoxelConnectivityFilter)
{
    VolProc::VoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
