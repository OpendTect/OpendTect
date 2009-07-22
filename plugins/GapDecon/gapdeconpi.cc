/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id";

#include "gapdeconattrib.h"
#include "plugins.h"

mExternC int GetGapDeconPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetGapDeconPluginInfo()
{
    static PluginInfo retpii = {
	"Gap Decon Base",
	"dGB - Helene",
	"=od",
	"Gap Decon (Prediction Error Filter) attribute plugin.\n\n" };
    return &retpii;
}


mExternC const char* InitGapDeconPlugin( int, char** )
{
    Attrib::GapDecon::initClass();

    return 0; // All OK - no error messages
}
