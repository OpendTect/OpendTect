/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/


#include "odplugin.h"

#include "externalattribrandom.h"

mDefODPluginEarlyLoad(ExternalAttrib)
mDefODPluginInfo(ExternalAttrib)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"External attribute example plugin (Base)",
	"OpendTect",
	"dGB Earth Sciences (Kristofer Tingdahl)",
	"=od",
	"Defining an external plugin with random numbers between 0 and 1." ))
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
