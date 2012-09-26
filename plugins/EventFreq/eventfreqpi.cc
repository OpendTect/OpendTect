/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "eventfreqattrib.h"
#include "odplugin.h"

mDefODPluginEarlyLoad(EventFreq)
mDefODPluginInfo(EventFreq)
{
    static PluginInfo retpii = {
	"Event frequency Attribute (Base)",
	"dGB - Bert Bril",
	"=od",
	"Defining the 'EventFreq' attribute." };
    return &retpii;
}


mDefODInitPlugin(EventFreq)
{
    Attrib::EventFreq::initClass();
    return 0; // All OK - no error messages
}
