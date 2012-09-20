/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Unit IDs
-*/
 
static const char* rcsID mUnusedVar = "$Id$";

#include "multiid.h"
#include "globexpr.h"
#include <stdlib.h>
#include <iostream>


static char bufstr[4096];


// returns piece from level nr, copies that part only if tobuf == true
char* CompoundKey::fromKey( int keynr, bool tobuf ) const
{
    bufstr[0] = '\0';
    if ( keynr<1 && !tobuf ) return (char*) id_.buf();

    const char* ptr = (const char*)id_;
    if ( !ptr ) return 0;

    while ( keynr )
    {
	ptr = strchr( ptr, '.' );
	if ( !ptr ) return 0;
	ptr++; keynr--;
    }

    if ( tobuf )
    {
	strcpy( bufstr, ptr );
	char* ptrend = strchr( bufstr, '.' );
	if ( ptrend ) *ptrend = '\0';
    }

    return (char*)ptr;
}


int CompoundKey::nrKeys() const
{
    if ( id_.isEmpty() ) return 0;

    int nrkeys = 1;
    const char* ptr = id_;
    while ( ( ptr=strchr(ptr,'.') ) ) { nrkeys++; ptr++; }

    return nrkeys;
}


BufferString CompoundKey::key( int idx ) const
{
    fromKey( idx, true );
    return BufferString(bufstr);
}


void CompoundKey::setKey( int ikey, const char* s )
{
    char* ptr = fromKey( ikey );
    if ( !ptr ) return;
    char* endptr = strchr( ptr, '.' );
    if ( !endptr ) { strcpy( ptr, s ); return; }
    BufferString rest( endptr );
    strcpy( ptr, s );
    strcat( ptr, rest );
}


CompoundKey CompoundKey::upLevel() const
{
    if ( id_.isEmpty() ) return CompoundKey("");
    CompoundKey newid( *this );

    int nrkeys = nrKeys();
    if ( nrkeys <= 1 )
	newid = "";
    else
    {
	char* ptr = newid.fromKey( nrkeys-1, false );
	if ( ptr ) *(ptr-1) = '\0';
    }

    return newid;
}


bool CompoundKey::isUpLevelOf( const CompoundKey& ky ) const
{
    return nrKeys() < ky.nrKeys() && matchString( id_, ky.id_ );
}


bool CompoundKey::matchGE( const char* cre ) const
{
    return GlobExpr(id_).matches(cre);
}


int MultiID::leafID() const
{
    const char* ptr = strrchr( id_, '.' );
    return toInt( ptr ? ptr+1 : (const char*)id_ );
}


const MultiID& MultiID::udf()
{
   static MultiID _udf( -1 );
   return _udf;
}


std::ostream& operator <<( std::ostream& strm, const CompoundKey& ck )
{
    strm << ck.buf(); return strm;
}


std::istream& operator >>( std::istream& strm, CompoundKey& ck )
{
    strm >> ck.id_; return strm;
}
