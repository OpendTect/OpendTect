/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"

#include "externalattribrandom.h"

mDefODPluginInfo(ExternalAttrib)
{
    static PluginInfo retpii = {
	"External attribute example plugin",
	"dGB - Kristofer Tingdahl",
	"=od",
	"Defining an external plugin with random numbers between 0 and 1." };
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
