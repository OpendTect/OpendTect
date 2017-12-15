/*+
________________________________________________________________________

* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Paul
* DATE	   : Dec 2012
________________________________________________________________________
-*/


#include "uiceemdattribmod.h"
#include "uiceemdattrib.h"
#include "odplugin.h"
#include <iostream>


mDefODPluginInfo(uiCEEMDAttrib)
{
    mDefineStaticLocalObject (PluginInfo, retpi, (
	"CEEMD: Complete Ensemble Empirical Mode Decomposition",
	mODPluginExtraAttribsPackage,
	mODPluginCreator, mODPluginVersion,
	"Provides the Complete Ensemble Empirical Mode Decomposition "
		"Attribute" ) );
    return &retpi;
}


mDefODInitPlugin(uiCEEMDAttrib)
{
    uiCEEMDAttrib::initClass();
    return 0;
}
