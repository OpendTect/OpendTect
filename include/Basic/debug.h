#ifndef debug_h
#define debug_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Lammertink
 Date:		Jun 2003
 RCS:		$Id: debug.h,v 1.15 2012-08-03 13:00:11 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"  
 
/*!\brief defines a generic interface for supplying debug/runtime info

    The isOn() is controlled by the environment variable DTECT_DEBUG.
    If DTECT_DEBUG starts with a "Y" or "y" then the mask is set to 0xffff.
    isOn returns the bitwise "and" of the passed flag and the internal
    environment mask, converted to a boolean.

    The reserved/defined masks are defined in "debugmasks.h".
    If you want, you can include it in a .cc you want to spit out debug
    messages, with the risk of regular big recompiles if someone adds a
    new debugmask.
    You can also make sure the debugmask you're using is defined in one
    of your header files and equals the one in "debugmasks.h", so you 
    don't have to include "debugmasks.h".

    if you want the output in a file, set the full path in DTECT_DEBUG_LOGFILE.

*/

# ifdef __cpp__
namespace DBG
{
    mGlobal(Basic) bool isOn( int flag=0xffff ); 

    mGlobal(Basic) void message( const char* ); 		    // default: to stderr
    mGlobal(Basic) void message( int flag, const char* msg ); 
// { if ( isOn(flag) ) message(msg); }
    mGlobal(Basic) void putProgInfo(int,char**); 		    //!< one line; more if isOn()
    mGlobal(Basic) void forceCrash(bool withdump); 
};

extern "C" {
# endif

    mGlobal(Basic) int od_debug_isOn( int flag );
    mGlobal(Basic) void od_debug_message( const char* msg );
    mGlobal(Basic) void od_debug_messagef( int flag, const char* msg );
    mGlobal(Basic) void od_debug_putProgInfo(int,char**);
    mGlobal(Basic) void od_putProgInfo(int,char**);

# ifdef __cpp__
}
# endif
#endif

