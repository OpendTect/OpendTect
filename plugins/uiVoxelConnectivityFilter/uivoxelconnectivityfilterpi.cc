/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2011
-*/


#include "odplugin.h"

#include "uivoxelconnectivityfilter.h"

mDefODPluginInfo(uiVoxelConnectivityFilter)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"VoxelConnectivityFilter (GUI)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"(c) dGB Beheer BV.\nDevelopment funded by Tetrale Technologies." ))
    return &retpi;
}


mDefODInitPlugin(uiVoxelConnectivityFilter)
{
    VolProc::uiVoxelConnectivityFilter::initClass();
    return nullptr; // All OK - no error messages
}
