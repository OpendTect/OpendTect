
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "tutmod.h"
#include "tutorialattrib.h"
#include "tutvolproc.h"
#include "odplugin.h"

mExternC(Tut) int GetTutPluginType();
mExternC(Tut) PluginInfo* GetTutPluginInfo();
mExternC(Tut) const char* InitTutPlugin(int,char**);


int GetTutPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


PluginInfo* GetTutPluginInfo()
{
    mDefineStaticLocalObject( PluginInfo, info, );
    info.dispname_ = "Tutorial plugin (Base)";
    info.productname_ = "Tutorial";
    info.creator_ = "dGB (Raman/Bert)";
    info.version_ = "3.2";
    info.text_ =
	"Back-end for the plugin that shows simple plugin development basics.\n"
	"This non-UI part can also be loaded into od_process_attrib.";
    return &info;
}


const char* InitTutPlugin( int argc, char** argv )
{
    Attrib::Tutorial::initClass();
    VolProc::TutOpCalculator::initClass();

    return nullptr;
}
