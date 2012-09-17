/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: expattribspi.cc,v 1.6 2011/04/21 13:09:13 cvsbert Exp $";

#include "odplugin.h"

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
