/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"

#include "expattribsmod.h"

#include "semblanceattrib.h"
#include "grubbsfilterattrib.h"


mDefODPluginEarlyLoad(ExpAttribs)
mDefODPluginInfo(ExpAttribs)
{
    static PluginInfo retpi = {
	"Experimental Attributes (Non-UI)",
	"dGB (Nanne)",
	"=od",
    	"" };
    return &retpi;
}


mDefODInitPlugin(ExpAttribs)
{
    Attrib::Semblance::initClass();
    Attrib::GrubbsFilter::initClass();
    return 0;
}
