/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/

static const char* rcsID = "$Id: strmoper.cc,v 1.13 2004-04-27 15:51:15 bert Exp $";

#include "strmoper.h"
#include "strmprov.h"
#include "timefun.h"
#include "errh.h"
#include <iostream>


std::istream* openInputStream( const char* fname )
{
    if ( !fname ) return &std::cin;

    StreamProvider sp( fname );
    return sp.makeIStream().istrm;
}


std::ostream* openOutputStream( const char* fname )
{
    if ( !fname ) return &std::cout;
    StreamProvider sp( fname );
    return sp.makeOStream().ostrm;
}


void closeIOStream( std::ostream*& streamptr )
{
    if ( streamptr != &std::cout && streamptr != &std::cerr ) delete streamptr;
    streamptr = 0;
}


void closeIOStream( std::istream*& streamptr )
{
    if ( streamptr != &std::cin ) delete streamptr;
    streamptr = 0;
}


bool writeWithRetry( std::ostream& strm, const void* ptr, unsigned int nrbytes,
		     unsigned int nrretries, unsigned int delay )
{
    if ( strm.bad() || !ptr ) return false;
    strm.clear();

    strm.write( (const char*)ptr, nrbytes );
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
	    strm.write( (const char*)ptr, nrbytes );
	    if ( !strm.fail() )
		break;
	}
	strm.flush();
    }

    return strm.good();
}


bool readWithRetry( std::istream& strm, void* ptr, unsigned int nrbytes,
		    unsigned int nrretries, unsigned int delay )
{
    if ( strm.bad() || strm.eof() || !ptr ) return false;
    strm.clear();

    strm.read( (char*)ptr, nrbytes );
    if ( strm.bad() ) return false;

    nrbytes -= strm.gcount();
    if ( nrbytes > 0 )
    {
	if ( strm.eof() ) return false;

	char* cp = (char*)ptr + strm.gcount();
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


bool wordFromLine( std::istream& strm, char* ptr, int maxnrchars )
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


bool ignoreToEOL( std::istream& strm )
{
    int c;
    while ( 1 )
    {
	c = strm.peek();
	if ( !strm.good() || c == '\n' )
	     break;
	strm.ignore( 1 );
    }
    strm.ignore( 1 );
    return strm.good();
}
