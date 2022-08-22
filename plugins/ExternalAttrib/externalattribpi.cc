/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "externalattribrandom.h"

mDefODPluginEarlyLoad(ExternalAttrib)
mDefODPluginInfo(ExternalAttrib)
{
    static PluginInfo retpi(
	"External attribute example plugin (Base)",
	"Defining an external plugin with random numbers between 0 and 1." );
    return &retpi;
}


static PtrMan<ExternalAttrib::RandomManager> randommanager;


mDefODInitPlugin(ExternalAttrib)
{
    ExternalAttrib::uiRandomTreeItem::initClass();
    ExternalAttrib::Random::initClass();

    randommanager.createIfNull( new ExternalAttrib::RandomManager() );

    return nullptr; // All OK - no error messages
}
