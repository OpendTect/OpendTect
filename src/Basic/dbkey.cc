/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "dbkey.h"
#include "compoundkey.h"
#include "bufstringset.h"


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

    if ( !isValidGroupedIDString( inpstr.str() ) )
	return;

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



DBKey::DBKey( const char* str )
    : auxkey_(0)
{
    fromString( str );
}


DBKey::~DBKey()
{
    delete auxkey_;
}


DBKey& DBKey::operator =( const DBKey& oth )
{
    if ( this != &oth )
    {
	IDWithGroup<int,int>::operator =( oth );
	setAuxKey( oth.auxkey_ ? oth.auxkey_->str() : 0 );
    }
    return *this;
}


bool DBKey::operator ==( const DBKey& oth ) const
{
    if ( !IDWithGroup<int,int>::operator ==(oth) )
	return false;

    const bool haveauxkey = auxkey_;
    const bool othhasauxkey = oth.auxkey_;
    if ( !haveauxkey && !othhasauxkey )
	return true;
    else if ( !haveauxkey || !othhasauxkey )
	return false;

    return *auxkey_ == *oth.auxkey_;
}


BufferString DBKey::toString() const
{
    BufferString ret;

    if ( groupnr_ < 0 )
	ret.set( -1 );
    else
    {
	ret.set( groupnr_ );
	if ( objnr_ > 0 )
	    ret.add( "." ).add( objnr_ );
    }

    if ( auxkey_ )
	ret.add( "|" ).add( *auxkey_ );

    return ret;
}


void DBKey::fromString( const char* str )
{
    od_int64 gnr, onr; BufferString aux;
    getGroupedIDNumbers( str, gnr, onr, &aux );
    groupnr_ = (GroupNrType)gnr;
    objnr_ = (ObjNrType)onr;
    setAuxKey( aux );
}


DBKey DBKey::getFromString( const char* str )
{
    DBKey ret = getInvalid();
    ret.fromString( str );
    return ret;
}


BufferString DBKey::auxKey() const
{
    return auxkey_ ? *auxkey_ : BufferString::empty();
}


void DBKey::setAuxKey( const char* str )
{
    if ( !str || !*str )
	{ delete auxkey_; auxkey_ = 0; }
    else
    {
	if ( !auxkey_ )
	    auxkey_ = new BufferString( str );
	else
	    *auxkey_ = str;
    }
}



DBKey DBKey::getFromInt64( od_int64 i64 )
{
    GroupedID id = GroupedID::getInvalid();
    id.fromInt64( i64 );
    return DBKey( id.groupNr(), id.objNr() );
}


uiString DBKey::toUiString() const
{
    return ::toUiString( toString() );
}


void DBKeySet::addTo( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<size(); idx++ )
	bss.add( ((*this)[idx]).toString() );
}
