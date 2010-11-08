
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tutpi.cc,v 1.7 2010-11-08 11:48:22 cvsbert Exp $";

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
	"Tutorial plugin Base",
	"dGB (Raman/Bert)",
	"3.2",
    	"Back-end for the plugin that shows simple plugin development basics."
    	"\nThis non-UI part can also be loaded into od_process_attrib." };
    return &retpi;
}


mExternC const char* InitTutPlugin( int, char** )
{
    Attrib::Tutorial::initClass();

    return 0;
}
