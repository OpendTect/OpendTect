/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
 * SVN      : $Id$
-*/

#include "uieventfreqattrib.h"
#include "odplugin.h"


mDefODPluginInfo(uiEventFreq)
{
    static PluginInfo retpi = {
	"Event Frequency",
	"dGB",
	"=od",
	"User interface for Event Frequency attribute." };
    return &retpi;
}


mDefODInitPlugin(uiEventFreq)
{
    uiEventFreqAttrib::initClass();
    return 0;
}
