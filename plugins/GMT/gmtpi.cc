/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.9 2012/03/19 13:24:34 cvsnageswara Exp $";

#include "initgmtplugin.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(GMT)
mDefODPluginInfo(GMT)
{
    static PluginInfo retpi = {
	"GMT (base)",
	"dGB (Raman)",
	"=od",
    	"GMT mapping tool - base" };
    return &retpi;
}


mDefODInitPlugin(GMT)
{
    GMT::initStdClasses();

    return 0;
}
