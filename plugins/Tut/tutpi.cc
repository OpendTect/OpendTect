
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "odplugin.h"

#include "tutmod.h"
#include "tutorialattrib.h"
#include "tutvolproc.h"

mDefODPluginEarlyLoad(Tut)
mDefODPluginInfo(Tut)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Tutorial plugin (Base)",
	"OpendTect",
	"dGB Earth Sciences (Raman/Bert)",
	"=od",
	"Back-end for the plugin that shows simple plugin development basics.\n"
	"This non-UI part can also be loaded into od_process_attrib." ))
    return &retpi;
}


mDefODInitPlugin(Tut)
{
    Attrib::Tutorial::initClass();
    VolProc::TutOpCalculator::initClass();

    return nullptr;
}
