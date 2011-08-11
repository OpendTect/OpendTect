/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: uivoxelconnectivityfilterpi.cc,v 1.1 2011-08-11 09:46:19 cvskris Exp $";

#include "plugins.h"

#include "uivoxelconnectivityfilter.h"

mExternC int GetuiVoxelConnectivityFilterPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiVoxelConnectivityFilterPluginInfo()
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin (UI)",
	"Kristofer",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mExternC const char* InituiVoxelConnectivityFilterPlugin( int, char** )
{
    VolProc::uiVoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
