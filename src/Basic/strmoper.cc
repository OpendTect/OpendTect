/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/

static const char* rcsID = "$Id: strmoper.cc,v 1.6 2000-06-23 14:11:10 bert Exp $";

#include "strmoper.h"
#include "strmprov.h"
#include "timefun.h"
#include "errh.h"
#include <iostream.h>


istream* openInputStream( const char* fname )
{
    if ( !fname ) return &cin;

    StreamProvider sp( fname );
    return sp.makeIStream().istrm;
}


ostream* openOutputStream( const char* fname )
{
    if ( !fname ) return &cout;
    StreamProvider sp( fname );
    return sp.makeOStream().ostrm;
}


void closeIOStream( ostream*& streamptr )
{
    if ( streamptr != &cout && streamptr != &cerr ) delete streamptr;
    streamptr = 0;
}


void closeIOStream( istream*& streamptr )
{
    if ( streamptr != &cin ) delete streamptr;
    streamptr = 0;
}


bool writeWithRetry( ostream& strm, const void* ptr, unsigned int nrbytes,
		     unsigned int nrretries, unsigned int delay )
{
    if ( strm.bad() ) return false;
    strm.clear();

    strm.write( ptr, nrbytes );
    if ( strm.fail() )
    {
	strm.flush();
	for ( int idx=0; idx<nrretries; idx++ )
	{
	    BufferString msg( "Soft error during write. Retrying after " );
	    msg += (int) delay;
	    msg += " msecs ...";
	    ErrMsg( msg );

	    Time_sleep( 0.001 * delay );
	    strm.clear();
	    strm.write( ptr, nrbytes );
	    if ( !strm.fail() )
		break;
	}
	strm.flush();
    }

    return strm.good();
}


bool readWithRetry( istream& strm, void* ptr, unsigned int nrbytes,
		    unsigned int nrretries, unsigned int delay )
{
    if ( strm.bad() || strm.eof() ) return false;
    strm.clear();

    strm.read( ptr, nrbytes );
    if ( strm.bad() ) return false;

    nrbytes -= strm.gcount();
    if ( nrbytes > 0 )
    {
	if ( strm.eof() ) return false;

	unsigned char* cp = (unsigned char*)ptr + strm.gcount();
	for ( int idx=0; idx<nrretries; idx++ )
	{
	    BufferString msg( "Soft error during read. Retrying after " );
	    msg += (int) delay;
	    msg += " msecs ...";
	    ErrMsg( msg );

	    Time_sleep( 0.001 * delay );
	    strm.clear();
	    strm.read( cp, nrbytes );
	    if ( strm.bad() || strm.eof() ) break;

	    nrbytes -= strm.gcount();
	    if ( !nrbytes )
		{ strm.clear(); break; }

	    cp += strm.gcount();
	}
    }

    return nrbytes ? false : true;
}


bool wordFromLine( istream& strm, char* ptr, int maxnrchars )
{
    if ( !ptr ) return NO;
    *ptr = '\0';

    char c;
    char* start = ptr;
    while ( strm )
    {
	c = strm.peek();
	if ( !isspace(c) )
	    *ptr++ = c;
	else if ( c == '\n' || *start )
	    break;

	maxnrchars--; if ( maxnrchars == 0 ) break;
	strm.ignore( 1 );
    }

    *ptr = '\0';
    return ptr != start;
}
