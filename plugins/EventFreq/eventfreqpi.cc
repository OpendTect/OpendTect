/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* mUnusedVar rcsID = "$Id: eventfreqpi.cc,v 1.4 2012-05-02 11:52:44 cvskris Exp $";

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
