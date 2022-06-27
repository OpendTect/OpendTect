/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003 / Apr 2011
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
