
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.5 2009-07-22 16:01:27 cvsbert Exp $";

#include "tutseistools.h"
#include "tutorialattrib.h"
#include "plugins.h"

mExternC mGlobal int GetTutPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC mGlobal PluginInfo* GetTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development (Non-UI)",
	"dGB (Raman/Bert)",
	"3.0",
    	"Shows some simple plugin basics." };
    return &retpi;
}


mExternC mGlobal const char* InitTutPlugin( int, char** )
{
    Attrib::Tutorial::initClass();

    return 0;
}
