/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horattribpi.cc,v 1.4 2008-02-28 12:25:56 cvsnanne Exp $
________________________________________________________________________

-*/


#include "horizonattrib.h"
#include "initearthmodel.h"
#include "plugins.h"


extern "C" int GetHorizonAttribPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetHorizonAttribPluginInfo()
{
    static PluginInfo retpii = {
	"Horizon-Attribute Base",
	"dGB (Nanne)",
	"=od",
	"The 'Horizon' attribute plugin." };
    return &retpii;
}


extern "C" const char* InitHorizonAttribPlugin( int, char** )
{
    EarthModel::initStdClasses();
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
