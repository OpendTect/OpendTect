/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Aug 2006
-*/

static const char* mUnusedVar rcsID = "$Id: externalattribpi.cc,v 1.4 2012-05-02 11:52:45 cvskris Exp $";

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
