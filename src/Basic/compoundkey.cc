/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Unit IDs
-*/
 
static const char* rcsID = "$Id: compoundkey.cc,v 1.2 2001-03-30 08:52:54 bert Exp $";

#include "multiid.h"
#include "globexpr.h"
#include <stdlib.h>


static char bufstr[4096];


// returns piece from level nr, copies that part only if tobuf == true
char* CompoundKey::fromKey( int keynr, bool tobuf ) const
{
    bufstr[0] = '\0';
    if ( keynr<1 && !tobuf ) return (char*)((const char*)id);

    const char* ptr = (const char*)id;
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
    if ( id == "" ) return 0;

    int nrkeys = 1;
    const char* ptr = id;
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
    if ( id == "" ) return CompoundKey("");
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
    return nrKeys() < ky.nrKeys() && matchString( id, ky.id );
}


bool CompoundKey::matchGE( const char* cre ) const
{
    return GlobExpr(id).matches(cre);
}


int MultiID::leafID() const
{
    const char* ptr = strrchr( id, '.' );
    return atoi( ptr ? ptr+1 : (const char*)id );
}
