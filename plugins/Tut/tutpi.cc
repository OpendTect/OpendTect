
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.6 2009-09-24 10:42:40 cvsnanne Exp $";

#include "tutseistools.h"
#include "tutorialattrib.h"
#include "plugins.h"

mExternC int GetTutPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development (Non-UI)",
	"dGB (Raman/Bert)",
	"3.0",
    	"Shows some simple plugin basics." };
    return &retpi;
}


mExternC const char* InitTutPlugin( int, char** )
{
    Attrib::Tutorial::initClass();

    return 0;
}
