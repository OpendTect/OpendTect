/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		June 2022
________________________________________________________________________

-*/


#include "groupedid.h"
#include "compoundkey.h"



bool isValidGroupedIDString( const char* str )
{
    if ( !str || !*str )
	return false;
    const bool isudf = *str == '-';
    if ( isudf )
	str++;

    bool digitseen = false;
    bool dotseen = false;
    while ( *str )
    {
	if ( iswdigit(*str) )
	    digitseen = true;
	else if ( *str == '|' )
	    return isudf ? digitseen : true;
	else if ( *str != '.' )
	    return false;
	else if ( dotseen )
	    return false;
	else
	    { dotseen = true; digitseen = false; }
	str++;
    }

    return digitseen;
}


void getGroupedIDNumbers( const char* str, od_int64& gnr, od_int64& onr,
			  BufferString* auxpart, BufferString* survpart )
{
    gnr = onr = -1;
    BufferString inpstr( str );
    if ( inpstr.isEmpty() )
	return;

    char* ptrbq = inpstr.find( '`' );
    if ( ptrbq )
    {
	*ptrbq = '\0';
	if ( survpart )
	    survpart->set( ptrbq + 1 );
    }

    char* ptrpipe = inpstr.find( '|' );
    if ( ptrpipe )
    {
	*ptrpipe = '\0';
	if ( auxpart )
	    auxpart->set( ptrpipe + 1 );
    }

    if ( !isValidGroupedIDString( inpstr.str() ) )
	return;

    CompoundKey ck( inpstr );
    const int len = ck.nrKeys();
    for ( auto idx : {0,1} )
	if ( idx < len )
	    (idx == 0 ? gnr : onr) = toInt64( ck.key(idx) );
}
