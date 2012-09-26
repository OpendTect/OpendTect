/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "voxelconnectivityfilter.h"

mDefODPluginEarlyLoad(VoxelConnectivityFilter);
mDefODPluginInfo(VoxelConnectivityFilter)
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin (base)",
	"dGB (Kristofer)",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mDefODInitPlugin(VoxelConnectivityFilter)
{
    VolProc::VoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
