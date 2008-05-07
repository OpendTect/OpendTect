/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id: externalattribpi.cc,v 1.1 2008-05-07 20:16:40 cvskris Exp $";

#include "plugins.h"

#include "externalattribrandom.h"

extern "C" int GetMatchDeltaPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetExternalAttribPluginInfo()
{
    static PluginInfo retpii = {
	"External attribute example plugin",
	"dGB - Kristofer Tingdahl",
	"=od",
	"Defining an external plugin with random numbers between 0 and 1." };
    return &retpii;
}


static PtrMan<ExternalAttrib::RandomManager> randommanager = 0;


extern "C" const char* InitExternalAttribPlugin( int, char** )
{
    ExternalAttrib::uiRandomTreeItem::initClass();
    ExternalAttrib::Random::initClass();

    if ( !randommanager )
	randommanager = new ExternalAttrib::RandomManager();

    return 0; // All OK - no error messages
}
