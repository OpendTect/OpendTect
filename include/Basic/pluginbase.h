#ifndef pluginbase_h
#define pluginbase_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; No license.
 Author:	A.H.Bril
 Date:		June 2006
 RCS:		$Id$
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

/*!\brief Information about plugin for outside world */

struct PluginInfo
{
    PluginInfo(const char* dispname, const char* creator,
               const char* version, const char* text)
        : dispname_( dispname )
        , creator_( creator )
        , version_( version )
        , text_( text )
    {}

    const char*	dispname_;
    const char*	creator_;
    const char*	version_;
    const char*	text_;

};

/* } -- for the extern "C" */

#endif
