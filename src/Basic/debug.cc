/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id: debug.cc,v 1.3 2003-10-15 09:12:57 arend Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: debug.cc,v 1.3 2003-10-15 09:12:57 arend Exp $";

#include "debug.h"
#include "debugmasks.h"
#include "bufstring.h"

#include <iostream>

namespace DBG
{

static int getMask()
{
    BufferString envmask = getenv("dGB_DEBUG");
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
    cout << msg << endl;
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


