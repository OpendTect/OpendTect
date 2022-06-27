/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "odplugin.h"
#include "voxelconnectivityfilter.h"

mDefODPluginEarlyLoad(VoxelConnectivityFilter)
mDefODPluginInfo(VoxelConnectivityFilter)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"VoxelConnectivityFilter (Base)",
	"OpendTect",
	"dGB Earth Sciences (Kristofer)",
	"=od",
	"(c) dGB Beheer BV.\nDevelopment funded by Tetrale Technologies." ))
    return &retpi;
}


mDefODInitPlugin(VoxelConnectivityFilter)
{
    VolProc::VoxelConnectivityFilter::initClass();
    return nullptr; // All OK - no error messages
}
