/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: voxelconnectivityfilterpi.cc,v 1.2 2011/08/11 09:54:15 cvskris Exp $";

#include "odplugin.h"
#include "voxelconnectivityfilter.h"

mDefODPluginEarlyLoad(VoxelConnectivityFilter);
mDefODPluginInfo(VoxelConnectivityFilter)
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin",
	"Kristofer",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mDefODInitPlugin(VoxelConnectivityFilter)
{
    VolProc::VoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
