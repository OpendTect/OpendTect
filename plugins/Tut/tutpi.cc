/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "odplugin.h"
#include "tutorialattrib.h"
#include "tutvolproc.h"

#if __has_include("tutversion.h")
# include "tutversion.h"
#endif

static const char* getProductName()
{
#ifdef Tut_PRODUCT_NAME
    return Tut_PRODUCT_NAME;
#else
    return "OpendTect";
#endif
}

static const char* getCreatorNm()
{
#ifdef Vendor
    return Vendor;
#else
    return "dGB Earth Sciences";
#endif
}

static const char* getVersion()
{
#ifdef Tut_VERSION
    return Tut_VERSION;
#else
    return "=od";
#endif
}

static const char* dispName()	    { return "Tutorial plugin (Base)"; }

static const char* dispText()
{
    return "Back-end for the plugin that shows simple plugin development basics.\n"
	   "This non-UI part can also be loaded into od_process_attrib.";
}

static PluginInfo::LicenseType getLicType()
{
#ifdef Vendor
    return PluginInfo::LicenseType::COMMERCIAL;
#else
    return PluginInfo::LicenseType::GPL;
#endif
}


mDefODPluginEarlyLoad(Tut)
mDefODPluginInfo(Tut)
{
    static PluginInfo retpi(
	dispName(),
	getProductName(),
	getCreatorNm(),
	getVersion(),
	dispText(),
	getLicType() );
    return &retpi;
}


mDefODInitPlugin(Tut)
{
    Attrib::Tutorial::initClass();
    VolProc::TutOpCalculator::initClass();

    return nullptr;
}
