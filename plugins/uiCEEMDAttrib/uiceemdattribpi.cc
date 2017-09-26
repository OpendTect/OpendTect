/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/

#include "uiceemdattribmod.h"
#include "uiceemdattrib.h"
#include "odplugin.h"
#include <iostream>


mDefODPluginInfo(uiCEEMDAttrib)
{
    mDefineStaticLocalObject (PluginInfo, retpi, (
	"uiCEEMD - Complete Ensemble Empirical Mode Decomposition"
	" Attribute Plugin",
	"OpendTect",
	"Paul de Groot",
	"=od",
	"User Interface for CEEMD Plugin" ));
    return &retpi;
}


mDefODInitPlugin(uiCEEMDAttrib)
{
    uiCEEMDAttrib::initClass();
    return 0;
}
