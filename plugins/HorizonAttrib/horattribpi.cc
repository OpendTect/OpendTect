/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horattribpi.cc,v 1.5 2008-11-25 15:35:21 cvsbert Exp $";


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
