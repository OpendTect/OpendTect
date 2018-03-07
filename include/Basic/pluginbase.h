#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; No license.
 Author:	Bert
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

Mandatory:
    dispname_	 -  The name shown in the PluginInfo window.
    packagename_ -  Multiple plugins can belong to one plugin package.
		    Can be shown in the plugin selection window.
    creator_	 - You
    version_	 - Use mODPluginVersion if the plugin runs with OD release.
    text_	 - Your description of the plugin and more.

Settable:
    LicenseType	 -  By default GPL.
    useronoffselectable_ - If this is set to true, the plugin selector will
		    expect entries in data/pkglist.txt and possibly an
		    entry in data/PluginProviders .
    url_	 - A URL specifically for this plugin.

*/

class uiString;


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
	, lictype_(lt)
        , url_(0)			{}

    const char*	dispname_;
    const char*	packagename_;
    const char*	creator_;
    const char*	version_;
    const char*	text_;

    const uiString*	uidispname_;
    const uiString*	uipackagename_;
    const char*		url_;
    bool		useronoffselectable_;
    LicenseType		lictype_;

};


// Not inline functions to make it possible to avoid compile-time deps on
// the uiString class.
// Example: mSetUserDisplayName( retpi, uiPresMakerPIMgr::dispNm() );

#define mSetPackageDisplayName(piinfo,nm) \
    (piinfo).uipackagename_ = new uiString( nm )

#define mSetDisplayName(piinfo,nm) \
    (piinfo).uidispname_ = new uiString( nm )

#define mGetPackageDisplayName(piinfo,uistr) \
    uistr = ((piinfo).uipackagename_ ? *(piinfo).uipackagename_ \
				     : toUiString( (piinfo).packagename_ ))
