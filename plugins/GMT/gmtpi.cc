/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.8 2012-03-06 06:21:22 cvsnageswara Exp $";

#include "initgmtplugin.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(GMT)
mDefODPluginInfo(GMT)
{
    static PluginInfo retpi = {
	"GMT Base",
	"dGB (Raman)",
	"3.2",
    	"GMT mapping tool - base" };
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMTPlugin::initStdClasses();

    return 0;
}
