/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Unit IDs
-*/
 
static const char* rcsID mUsedVar = "$Id$";

#include "multiid.h"
#include "globexpr.h"
#include "od_iostream.h"
#include "staticstring.h"
#include <stdlib.h>
#include <iostream>


char* CompoundKey::fromKey( int keynr ) const
{
    return fetchKeyPart( keynr, false );
}


char* CompoundKey::fetchKeyPart( int keynr, bool parttobuf ) const
{
    char* ptr = const_cast<char*>( id_.buf() );
    if ( !ptr || (keynr<1 && !parttobuf) )
	return ptr;

    while ( keynr > 0 )
    {
	ptr = strchr( ptr, '.' );
	if ( !ptr ) return 0;
	ptr++; keynr--;
    }

    if ( parttobuf )
    {
	mDeclStaticString(bufstr);
	bufstr = ptr; char* bufptr = bufstr.buf();
	char* ptrend = strchr( bufptr, '.' );
	if ( ptrend ) *ptrend = '\0';
	ptr = bufptr;
    }

    return ptr;
}


int CompoundKey::nrKeys() const
{
    if ( id_.isEmpty() ) return 0;

    int nrkeys = 1;
    const char* ptr = id_;
    while ( true )
    {
	ptr = strchr(ptr,'.');
	if ( !ptr )
	    break;
	nrkeys++;
	ptr++;
    }

    return nrkeys;
}


BufferString CompoundKey::key( int idx ) const
{
    return BufferString( fetchKeyPart(idx,true) );
}


void CompoundKey::setKey( int ikey, const char* s )
{
					// example: "X.Y.Z", ikey=1, s="new"

    char* ptr = fromKey( ikey );	// ptr="Y.Z"
    if ( !ptr ) return;

    const BufferString lastpart( strchr(ptr,'.') ); // lastpart=".Z"
    *ptr = '\0';			// id_="X."

    if ( s && *s )
	id_.add( s );			// id_="X.new"
    else if ( ptr != id_.buf() )
	*(ptr-1) = '\0';		// id_="X"

    id_.add( lastpart );		// id_="X.new.Z" or "X.Z" if s==0
}


CompoundKey CompoundKey::upLevel() const
{
    if ( id_.isEmpty() )
	return CompoundKey("");

    CompoundKey ret( *this );

    int nrkeys = nrKeys();
    if ( nrkeys <= 1 )
	ret = "";
    else
    {
	char* ptr = ret.fromKey( nrkeys-1 );
	if ( ptr ) *(ptr-1) = '\0';
    }

    return ret;
}


bool CompoundKey::isUpLevelOf( const CompoundKey& ky ) const
{
    return nrKeys() < ky.nrKeys() && matchString( id_, ky.id_ );
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
	{ strm << ck.buf(); return strm; }
std::istream& operator >>( std::istream& strm, CompoundKey& ck )
	{ strm >> ck.id_; return strm; }
