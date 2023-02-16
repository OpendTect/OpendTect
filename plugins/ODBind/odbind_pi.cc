/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "odbind.h"
#include "odplugin.h"
#include "ptrman.h"

mDefODPluginInfo(ODBind)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"ODBind plugin",
	"C callable interface for accessing OpendTect" ))
    return &retpi;
}


mDefODInitPlugin(ODBind)
{
    return nullptr;
}
