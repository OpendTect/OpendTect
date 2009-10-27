/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Helene Huck
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: madattribspi.cc,v 1.1 2009-10-27 15:55:06 cvshelene Exp $";

#include "madagcattrib.h"
#include "plugins.h"

mExternC int GetMadagascarAttribsPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetMadagascarAttribsPluginInfo()
{
    static PluginInfo retpii = {
	"Trace Match (Base)",
	"dGB - Helene Huck",
	"=od",
	"Transforming Madagascar routines into OpendTect attributes." };
    return &retpii;
}


mExternC const char* InitMadagascarAttribsPlugin( int, char** )
{
    Attrib::MadAGC::initClass();

    return 0; // All OK - no error messages
}
