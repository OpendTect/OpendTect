#ifndef debug_h
#define debug_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Lammertink
 Date:		Jun 2003
 RCS:		$Id: debug.h,v 1.4 2003-06-10 15:09:24 arend Exp $
________________________________________________________________________

-*/
 
 
/*!\brief defines a generic interface for supplying debug/runtime info

    The isOn() is controlled by the environment variable dGB_DEBUG.
    If dGB_DEBUG starts with a "Y" or "y" then the mask is set to 0xffff.
    isOn returns the bitwise "and" of the passed flag and the internal
    environment mask, converted to a boolean.

    The reserved/defined masks are defined in "debugmasks.h".
    If you want, you can include it in a .cc you want to spit out debug
    messages, with the risk of regular big recompiles if someone adds a
    new debugmask.
    You can also make sure the debugmask you're using is defined in one
    of your header files and equals the one in "debugmasks.h", so you 
    don't have to include "debugmasks.h".

*/

namespace DBG
{
    bool 		isOn( int flag=0xffff );

    void		message( const char* ); // default: to stdout
    void		message( int flag, const char* msg );
			   // { if ( isOn(flag) ) message(msg); }
};


#endif
