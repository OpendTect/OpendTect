/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: expattribspi.cc,v 1.7 2012-05-02 11:52:44 cvskris Exp $";

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
