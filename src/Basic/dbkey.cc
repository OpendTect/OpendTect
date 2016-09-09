/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "groupedid.h"
#include "compoundkey.h"


bool isValidGroupedIDString( const char* str )
{
    if ( !str || !*str || !iswdigit(*str) )
	return false;

    bool digitseen = false;
    bool dotseen = false;
    while ( *str )
    {
	if ( iswdigit(*str) )
	    digitseen = true;
	else if ( *str == '|' )
	    return digitseen;
	else if ( *str != '.' )
	    return false;
	else if ( dotseen )
	    return false;
	else
	    dotseen = true;
	str++;
    }

    return true;
}


void getGroupedIDNumbers( const char* str, od_int64& gnr, od_int64& onr,
			  BufferString* trailer )
{
    gnr = onr = -1;
    BufferString inpstr( str );
    if ( inpstr.isEmpty() )
	return;

    char* ptrtrailer = inpstr.find( '|' );
    if ( ptrtrailer )
	{ *ptrtrailer = '\0'; ptrtrailer++; }
    if ( trailer )
	trailer->set( ptrtrailer );

    CompoundKey ck( inpstr );
    const int len = ck.nrKeys();
    for ( int idx=0; idx<2; idx++ )
    {
	if ( idx < len )
	{
	    const BufferString subky( ck.key(idx) );
	    if ( idx == 0 )
		gnr = toInt64( subky.str() );
	    else
		onr = toInt64( subky.str() );
	}
    }
}
