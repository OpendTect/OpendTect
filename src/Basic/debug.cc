/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id: debug.cc,v 1.8 2004-11-29 10:57:25 bert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: debug.cc,v 1.8 2004-11-29 10:57:25 bert Exp $";

#include "debug.h"
#include "debugmasks.h"
#include "bufstring.h"

#include <iostream>
#include <fstream>

static std::ostream* dbglogstrm = 0;


namespace DBG
{

static int getMask()
{
    static bool maskgot = false;
    static int themask = 0;
    if ( maskgot ) return themask;
    maskgot = true;

    BufferString envmask = getenv( "DTECT_DEBUG" );
    const char* buf = envmask.buf();
    themask = atoi( buf );
    if ( buf[0] == 'y' || buf[0] == 'Y' ) themask = 0xffff;

    const char* dbglogfnm = getenv( "DTECT_DEBUG_LOGFILE" );
    if ( dbglogfnm && !themask )
	themask = 0xffff;

    if ( themask )
    {
	BufferString msg;
	if ( dbglogfnm )
	{
	    dbglogstrm = new std::ofstream( dbglogfnm );
	    if ( dbglogstrm && !dbglogstrm->good() )
	    {
		delete dbglogstrm; dbglogstrm = 0;
		msg = "Cannot open debug log file '";
		msg += dbglogfnm; msg == "': reverting to stdout";
		message( msg );
	    }
	}

	msg = "Debugging on, with mask: ";
	msg += themask;
	message( msg );
    }

    return themask;
}


bool isOn( int flag )
{
    static int mask = getMask();
    return flag & mask;
}



void message( const char* msg )
{
    if ( dbglogstrm )
	*dbglogstrm << msg << std::endl;
    else
    {
#ifdef __mac__
	printf( "%s\n", msg ); fflush( stdout );
#else
	std::cout << msg << std::endl;
#endif
    }
}


void message( int flag, const char* msg )
{
    if ( isOn(flag) ) message(msg);
}

};


extern "C" int od_debug_isOn( int flag )
    { return DBG::isOn(flag); }
extern "C" void od_debug_message( const char* msg )
    { DBG::message(msg); }
extern "C" void od_debug_messagef( int flag, const char* msg )
    { DBG::message(flag,msg); }
