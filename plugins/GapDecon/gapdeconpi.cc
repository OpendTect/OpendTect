/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"
#include "gapdeconattrib.h"

mDefODPluginEarlyLoad(GapDecon)
mDefODPluginInfo(GapDecon)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Gap Decon (base)",
	"dGB - Helene",
	"=od",
	"Gap Decon (Prediction Error Filter) attribute plugin.\n\n" ));
    return &retpi;
}


mDefODInitPlugin(GapDecon)
{
    Attrib::GapDecon::initClass();

    return 0; // All OK - no error messages
}
