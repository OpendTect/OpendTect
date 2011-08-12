/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: uivoxelconnectivityfilterpi.cc,v 1.2 2011-08-12 13:18:51 cvskris Exp $";

#include "odplugin.h"

#include "uivoxelconnectivityfilter.h"

mDefODPluginInfo(uiVoxelConnectivityFilter)
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin (UI)",
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
