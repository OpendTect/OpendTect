/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: expattribspi.cc,v 1.5 2011-03-24 03:00:29 cvsnanne Exp $";

#include "plugins.h"

#include "semblanceattrib.h"
#include "grubbsfilterattrib.h"


extern "C" int GetExpAttribsPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetExpAttribsPluginInfo()
{
    static PluginInfo retpi = {
	"Experimental Attributes (Non-UI)",
	"dGB (Nanne)",
	"=od",
    	"" };
    return &retpi;
}


extern "C" const char* InitExpAttribsPlugin( int, char** )
{
    Attrib::Semblance::initClass();
    Attrib::GrubbsFilter::initClass();
    return 0;
}
