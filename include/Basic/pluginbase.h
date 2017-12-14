#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; No license.
 Author:	A.H.Bril
 Date:		June 2006
________________________________________________________________________

 This header file is intended for programmers who want to use their code for
 both OpendTect plugins and programs not using OpendTect. You can freely add
 plugin init functions in your code without linking with OpendTect libs.

 Of course, if you _do_ link with the OpendTect libs, the OpendTect license
 becomes active.

-*/

#define PI_AUTO_INIT_NONE	0
#define PI_AUTO_INIT_EARLY	1
#define PI_AUTO_INIT_LATE	2

/*!\brief Information about plugin for outside world

    dispname_	 -  The name shown in the PluginInfo window.
    packagename_ -  Multiple plugins can belong to one plugin package.
		    shown in the plugin selection window.
    LicenseType	 -  By default GPL.
    useronoffselectable_ - If this is set to true, the plugin selector will
		    expect entries in data/pkglist.txt and possibly an
		    entry in data/PluginProviders .

    Note: Two different plugins(different display names) can have same package
    name if they belong to the same package

    e.g. Multi-Variate Analysis and Neural Networks both belong to the
    Neural Networks package.
*/


struct PluginInfo
{
    enum LicenseType	{ GPL, COMMERCIAL };

    PluginInfo( const char* dispname, const char* pkgnm, const char* creator,
                const char* version, const char* text, LicenseType lt=GPL )
        : dispname_(dispname)
	, packagename_(pkgnm)
        , creator_(creator)
        , version_(version)
        , text_(text)
        , useronoffselectable_(lt == COMMERCIAL)
	, lictype_(lt)	{}

    const char*	dispname_;
    const char*	packagename_;
    const char*	creator_;
    const char*	version_;
    const char*	text_;
    bool	useronoffselectable_;
    LicenseType lictype_;

};
