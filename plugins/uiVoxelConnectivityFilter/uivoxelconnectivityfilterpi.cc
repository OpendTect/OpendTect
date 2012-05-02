/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2011
-*/

static const char* mUnusedVar rcsID = "$Id: uivoxelconnectivityfilterpi.cc,v 1.4 2012-05-02 11:52:54 cvskris Exp $";

#include "odplugin.h"

#include "uivoxelconnectivityfilter.h"

mDefODPluginInfo(uiVoxelConnectivityFilter)
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin",
	"Kristofer",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mDefODInitPlugin(uiVoxelConnectivityFilter)
{
    VolProc::uiVoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
