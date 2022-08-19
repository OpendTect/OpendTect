/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hellomod.h"

#include "odplugin.h"
#include "od_ostream.h"

mDefODPluginEarlyLoad(Hello)
mDefODPluginInfo(Hello)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Hello World (Base)",
	"OpendTect",
	"Me",
	"1.0",
	"Hello World description" ))
    return &retpi;
}


mDefODInitPlugin(Hello)
{
    od_cout() << "Hello world" << od_endl;
    return nullptr; // All OK - no error messages
}
