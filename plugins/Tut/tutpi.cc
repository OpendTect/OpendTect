
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.4 2009-04-09 11:49:08 cvsranojay Exp $";

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
