/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id: eventfreqpi.cc,v 1.3 2011/04/21 13:09:13 cvsbert Exp $";

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
