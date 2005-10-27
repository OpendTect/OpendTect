/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.24 2005-10-27 15:04:44 cvsnanne Exp $";

#include "pickset.h"
#include "survinfo.h"
#include <ctype.h>

static double getNextVal( char*& str )
{
    if ( !*str ) return mUndefValue;
    char* endptr = str;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    if ( *endptr ) *endptr++ = '\0';
    double v = atof( str );
    str = endptr; skipLeadingBlanks(str);
    return v;
}


bool PickLocation::fromString( const char* s, bool doxy )
{
    if ( !s || !*s ) return false;

    if ( *s == '"' )
    {
	s++;
	text = new BufferString( s );
	char* start = text->buf();
	char* stop = strchr( start, '"' );
	if ( !stop )
	{
	    delete text;
	    text = 0;
	}
	else
	{
	    *stop = '\0';
	    s += stop - start + 1;
	}
    }

    BufferString bufstr( s );
    char* str = bufstr.buf();
    skipLeadingBlanks(str);

    double xread = getNextVal( str );
    double yread = getNextVal( str );
    double zread = getNextVal( str );
    if ( mIsUndefined(zread) )
	return false;

    pos.x = xread;
    pos.y = yread;
    z = zread;

    // Check if data is in inl/crl rather than X and Y
    if ( !SI().isReasonable(pos) || !doxy )
    {
	BinID bid( mNINT(pos.x), mNINT(pos.y) );
	SI().snap( bid, BinID(0,0) );
	Coord newpos = SI().transform( bid );
	if ( SI().isReasonable(newpos) )
	    pos = newpos;
    }

    // See if there's a direction, too
    xread = getNextVal( str );
    yread = getNextVal( str );
    if ( !mIsUndefined(yread) )
    {
	zread = getNextVal( str );
	if ( mIsUndefined(zread) ) zread = 0;
	dir = Sphere( xread, yread, zread );
    }

    return true;
}


void PickLocation::toString( char* str )
{
    if ( text )
    {
	strcpy( str, text->buf() );
	strcat( str, "\t" );
	strcat( str, getStringFromDouble(0,pos.x) );
    }
    else
	strcpy( str, getStringFromDouble(0,pos.x) );

    strcat( str, "\t" ); strcat( str, getStringFromDouble(0,pos.y) );
    strcat( str, "\t" ); strcat( str, getStringFromFloat(0,z) );

    if ( hasDir() )
    {
	strcat( str, "\t" ); strcat( str, getStringFromDouble(0,dir.radius) );
	strcat( str, "\t" ); strcat( str, getStringFromDouble(0,dir.theta) );
	strcat( str, "\t" ); strcat( str, getStringFromDouble(0,dir.phi) );
    }
}


void PickLocation::getKey( const char* idkey, BufferString& val ) const
{
    if ( !text )
    {
	val = "";
	return;
    }

    val = text->buf();
    char* start = strchr( val.buf(), idkey[0] );
    start++;
 
    char* stop = strchr( start, '\'' );
    *stop = '\0';
}


PickSet::PickSet( const char* nm )
	: UserIDObject(nm)
	, color(Color::NoColor)
{
}


void PickSetGroup::add( PickSet*& ps )
{
    if ( !ps ) return;
    const int pssz = ps->size();
    if ( !pssz ) { delete ps; return; }

    const int nrpss = sets.size();
    int mrgnr = -1;
    for ( int idx=0; idx<nrpss; idx++ )
	if ( ps->name() == sets[idx]->name() )
	    { mrgnr = idx; break; }
    if ( mrgnr < 0 ) { sets += ps; return; }

    PickSet& mrgps = *ps;
    ps = sets[mrgnr];

    for ( int idx=0; idx<pssz; idx++ )
    {
	if ( ps->indexOf( mrgps[idx] ) < 0 )
	    *ps += mrgps[idx];
    }

    delete &mrgps;
}
