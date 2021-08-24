/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003 / Apr 2011
-*/


#include "hellomod.h"

#include "odplugin.h"
#include "od_ostream.h"

mExternC(Hello) int GetHelloPluginType();
mExternC(Hello) PluginInfo* GetHelloPluginInfo();
mExternC(Hello) const char* InitHelloPlugin(int,char**);


int GetHelloPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


PluginInfo* GetHelloPluginInfo()
{
    mDefineStaticLocalObject( PluginInfo, info, );
    info.dispname_ = "Hello World";
    return &info;
}


const char* InitHelloPlugin( int argc, char** argv )
{
    od_cout() << "Hello world" << od_endl;
    return nullptr; // All OK - no error messages
}
