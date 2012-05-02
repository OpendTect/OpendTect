/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* mUnusedVar rcsID = "$Id: gapdeconpi.cc,v 1.8 2012-05-02 11:52:46 cvskris Exp $";

#include "odplugin.h"
#include "gapdeconattrib.h"

mDefODPluginEarlyLoad(GapDecon)
mDefODPluginInfo(GapDecon)
{
    static PluginInfo retpii = {
	"Gap Decon (base)",
	"dGB - Helene",
	"=od",
	"Gap Decon (Prediction Error Filter) attribute plugin.\n\n" };
    return &retpii;
}


mDefODInitPlugin(GapDecon)
{
    Attrib::GapDecon::initClass();

    return 0; // All OK - no error messages
}
