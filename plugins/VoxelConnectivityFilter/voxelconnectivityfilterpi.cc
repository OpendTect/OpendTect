/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: voxelconnectivityfilterpi.cc,v 1.1 2011-08-11 09:46:19 cvskris Exp $";

#include "plugins.h"

#include "voxelconnectivityfilter.h"

mExternC int GetVoxelConnectivityFilterPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetVoxelConnectivityFilterPluginInfo()
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin",
	"Kristofer",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mExternC const char* InitVoxelConnectivityFilterPlugin( int, char** )
{
    VolProc::VoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
