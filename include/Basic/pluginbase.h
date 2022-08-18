#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 This header file is intended for programmers who want to use their code for
 both OpendTect plugins and programs not using OpendTect. You can freely add
 plugin init functions in your code without linking with OpendTect libs.

 Of course, if you _do_ link with the OpendTect libs, the OpendTect license
 becomes active.

-*/

/* If you do not use OpendTect stuff here, you need a way to figure out whether
   you are in the C++ world and add something like:

#ifdef __cplusplus
extern "C" {
#endif
*/

#define PI_AUTO_INIT_NONE	0
#define PI_AUTO_INIT_EARLY	1
#define PI_AUTO_INIT_LATE	2

/*!\brief Information about plugin for outside world

    dispname_	 -  The name shown in the PluginInfo window.
    productname_ -  Same as the name of the commercial product as marketed to be
		    shown in the plugin selection window.
    LicenseType	 -  By default it is GPL, and will not be shown in the
		    plugin selection window.
		    Only commercial(COMMERCIAL)
		    products should be shown in the plugin selection window

    Note: Two different plugins(different display names) can have same product
    name if they belong to the same product

    e.g. Multi-Variate Analysis and Neural Networks both belong to the
    Neural Networks product.
*/


struct PluginInfo
{
    enum LicenseType{ GPL, COMMERCIAL };

    PluginInfo(const char* dispname, const char* prodnm, const char* creator,
	       const char* version, const char* text, LicenseType lt=GPL )
	: dispname_(dispname)
	, productname_(prodnm)
	, creator_(creator)
	, version_(version)
	, text_(text)
	, licmsg_("")
	, lictype_(lt)
    {}

    PluginInfo(const char* dispname, const char* prodnm, const char* creator,
	       const char* version, const char* text, const char* licmsg,
	       LicenseType lt=GPL )
	: dispname_(dispname)
	, productname_(prodnm)
	, creator_(creator)
	, version_(version)
	, text_(text)
	, licmsg_(licmsg)
	, lictype_(lt)
    {}

    PluginInfo()
	: dispname_("Plugin Name")
	, productname_("New Product")
	, creator_("A Developer")
	, version_("v0.0")
	, text_("Plugin information text")
	, licmsg_("License message")
	, lictype_(GPL)
    {}

    const char*	dispname_;
    const char*	productname_;
    const char*	creator_;
    const char*	version_;
    const char*	text_;
    const char*	licmsg_;
    LicenseType lictype_;
};

/* } -- for the extern "C" */
