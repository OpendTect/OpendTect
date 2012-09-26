/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID mUsedVar = "$Id";

#include "uigapdeconattrib.h"
#include "odplugin.h"

mDefODPluginInfo(uiGapDecon)
{
    static PluginInfo retpi = {
	"Gap Decon",
	"dGB (Helene)",
	"=od",
	"User Interface for Gap Decon attribute plugin." };
    return &retpi;
}


mDefODInitPlugin(uiGapDecon)
{
    uiGapDeconAttrib::initClass();
    return 0; // All OK - no error messages
}
