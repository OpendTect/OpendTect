/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id: debug.cc,v 1.7 2004-05-27 15:19:05 macman Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: debug.cc,v 1.7 2004-05-27 15:19:05 macman Exp $";

#include "debug.h"
#include "debugmasks.h"
#include "bufstring.h"

#include <iostream>

namespace DBG
{

static int getMask()
{
    BufferString envmask = getenv("DTECT_DEBUG");
    int res = atoi( envmask );

    const char* buf=envmask.buf();
    if ( buf[0] == 'y' || buf[0]=='Y' ) res = 0xffff;

    if( res )
    {
	BufferString msg("Debugging on, with mask: ");
	msg += res;
	message(msg);
    }

    return res;
}


bool isOn( int flag )
{
    static int mask = getMask();
    return flag & mask;
}



void message( const char* msg )
{
#ifdef __mac__
    printf( "%s\n", msg );
#else
    std::cout << msg << std::endl;
#endif
}


void message( int flag, const char* msg )
{
    if ( isOn(flag) ) message(msg);
}

};

extern "C"{

int dgb_debug_isOn( int flag )
    { return DBG::isOn(flag); }
void dgb_debug_message( const char* msg )
    { DBG::message(msg); }
void dgb_debug_messagef( int flag, const char* msg )
    { DBG::message(flag,msg); }

}


