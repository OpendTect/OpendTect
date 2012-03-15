/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: gmtpi.cc,v 1.10 2012-03-15 14:43:01 cvsbert Exp $";

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
