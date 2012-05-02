
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* mUnusedVar rcsID = "$Id: tutpi.cc,v 1.9 2012-05-02 11:52:47 cvskris Exp $";

#include "tutseistools.h"
#include "tutorialattrib.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(Tut)
mDefODPluginInfo(Tut)
{
    static PluginInfo retpi = {
	"Tutorial plugin Base",
	"dGB (Raman/Bert)",
	"3.2",
    	"Back-end for the plugin that shows simple plugin development basics."
    	"\nThis non-UI part can also be loaded into od_process_attrib." };
    return &retpi;
}


mDefODInitPlugin(Tut)
{
    Attrib::Tutorial::initClass();

    return 0;
}
