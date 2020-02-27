
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "tutseistools.h"
#include "tutorialattrib.h"
#include "tutvolproc.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(Tut)
mDefODPluginInfo(Tut)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Tutorial plugin (Base)",
	mODPluginTutorialsPackage,
	"dGB Earth Sciences",
	"=od",
	"Back-end for the plugin that shows simple plugin development basics."
	"\nThis non-UI part can also be loaded into batch programs." ) );
    return &retpi;
}


mDefODInitPlugin(Tut)
{
    Attrib::Tutorial::initClass();
    VolProc::TutOpCalculator::initClass();

    return 0;
}
