/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.30 2006-05-08 20:22:19 cvsnanne Exp $";

#include "pickset.h"
#include "survinfo.h"
#include "multiid.h"
#include <ctype.h>

Pick::SetGroupMgr* Pick::SetGroupMgr::theinst_ = 0;

Pick::SetGroupMgr& Pick::SGMgr()
{
    if ( !Pick::SetGroupMgr::theinst_ )
	Pick::SetGroupMgr::theinst_ = new Pick::SetGroupMgr;
    return *Pick::SetGroupMgr::theinst_;
}


Pick::Location::~Location()
{
    if ( text ) delete text;
}


void Pick::Location::operator=( const Pick::Location& pl )
{
    pos = pl.pos;
    z = pl.z;
    dir = pl.dir;
    if ( pl.text )
    {
	if ( !text )
	    text = new BufferString( *pl.text );
	else
	    *text = *pl.text;
    }
}


void Pick::Location::setText( const char* key, const char* txt )
{
    if ( !text ) text = new BufferString;
    *text += key; *text += "'"; *text += txt; *text += "'";
}


static double getNextVal( char*& str )
{
    if ( !*str ) return mUdf(double);
    char* endptr = str;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    if ( *endptr ) *endptr++ = '\0';
    double v = atof( str );
    str = endptr; skipLeadingBlanks(str);
    return v;
}


bool Pick::Location::fromString( const char* s, bool doxy )
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
    if ( mIsUdf(zread) )
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
    if ( !mIsUdf(yread) )
    {
	zread = getNextVal( str );
	if ( mIsUdf(zread) ) zread = 0;
	dir = Sphere( xread, yread, zread );
    }

    return true;
}


void Pick::Location::toString( char* str )
{
    if ( text )
    {
	strcpy( str, "\"" );
	strcat( str, text->buf() );
	strcat( str, "\"" );
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


void Pick::Location::getKey( const char* idkey, BufferString& val ) const
{
    if ( !text )
    {
	val = "";
	return;
    }

    BufferString res = text->buf();
    char* start = strchr( res.buf(), idkey[0] );
    start++; start++;
 
    char* stop = strchr( start, '\'' );
    *stop = '\0';
    val = start;
}


Pick::Set::Set( const char* nm )
    : UserIDObject(nm)
    , color_(Color::NoColor)
    , pixsize_(3)
{
}


void Pick::SetGroup::add( Pick::Set*& ps )
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

    Pick::Set& mrgps = *ps;
    ps = sets[mrgnr];

    for ( int idx=0; idx<pssz; idx++ )
    {
	if ( ps->indexOf( mrgps[idx] ) < 0 )
	    *ps += mrgps[idx];
    }

    delete &mrgps;
}


void Pick::SetGroupMgr::add( const MultiID& ky, SetGroup* sg )
{
    psgs_ += sg; ids_ += new MultiID( ky );
    itemAdded.trigger( sg );
}


void Pick::SetGroupMgr::set( const MultiID& ky, SetGroup* sg )
{
    SetGroup* oldsg = find( ky );
    if ( !oldsg )
    {
	if ( sg )
	    add( ky, sg );
    }
    else if ( sg != oldsg )
    {
	int idx = psgs_.indexOf( oldsg );
	itemToBeRemoved.trigger( oldsg );
	delete oldsg; psgs_.remove( idx );
	delete ids_[idx]; ids_.remove( idx );
	if ( sg )
	    add( ky, sg );
    }
}


Pick::SetGroup* Pick::SetGroupMgr::find( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == *ids_[idx] )
	    return const_cast<Pick::SetGroup*>( psgs_[idx] );
    }
    return 0;
}


MultiID* Pick::SetGroupMgr::find( const Pick::SetGroup& sg ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( &sg == psgs_[idx] )
	    return const_cast<MultiID*>( ids_[idx] );
    }
    return 0;
}
