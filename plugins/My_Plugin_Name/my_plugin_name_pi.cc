/*+
 * (C) Your_copyright
 * AUTHOR   : You!
 * DATE     : Apr 2012
-*/


#include "my_first_separate_source.h"
#include "odplugin.h"
#include "ptrman.h"


mDefODPluginInfo(My_Plugin_Name)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"My plugin (GUI)",
	"My Product",
	"You!",
	"My version",
	"My description ..."
	    "\n ... which can span many lines."
	    "\nDon't put commas between those lines though ...") );
    return &retpi;
}


mDefODInitPlugin(My_Plugin_Name)
{
    mDefineStaticLocalObject( PtrMan<My_Class>, theinst_,
	    = new My_Class() );

    return nullptr;
}
