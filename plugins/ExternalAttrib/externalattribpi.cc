/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2006
-*/

#include "odplugin.h"

#include "externalattribrandom.h"

mDefODPluginInfo(ExternalAttrib)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"External attribute example plugin",
	mODPluginTutorialsPackage,
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpii;
}


static PtrMan<ExternalAttrib::RandomManager> randommanager = 0;


mDefODInitPlugin(ExternalAttrib)
{
    ExternalAttrib::uiRandomTreeItem::initClass();
    ExternalAttrib::Random::initClass();

    if ( !randommanager )
	randommanager = new ExternalAttrib::RandomManager();

    return 0; // All OK - no error messages
}
