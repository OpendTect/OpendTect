#ifndef debug_h
#define debug_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Lammertink
 Date:		Jun 2003
 RCS:		$Id: debug.h,v 1.8 2004-11-29 10:57:25 bert Exp $
________________________________________________________________________

-*/

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
    bool 		isOn( int flag=0xffff );

    void		message( const char* ); // default: to stdout
    void		message( int flag, const char* msg );
			   // { if ( isOn(flag) ) message(msg); }
};

extern "C" {
# endif

int			od_debug_isOn( int flag );
void			od_debug_message( const char* msg );
void			od_debug_messagef( int flag, const char* msg );

# ifdef __cpp__
}
# endif
#endif
