/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "fulldbkey.h"
#include "bufstringset.h"
#include "compoundkey.h"
#include "filepath.h"


static BufferString noNameOfFn( const DBKey& ) { return BufferString(); }
static IOObj* noGetIOObjFn( const DBKey& ) { return 0; }

typedef BufferString (*nameOfFn)(const DBKey&);
typedef IOObj* (*getIOObjFn)(const DBKey&);

static nameOfFn nameoffn_ = noNameOfFn;
static getIOObjFn getioobjfn_ = noGetIOObjFn;

mGlobal(Basic) void setDBMan_DBKey_Fns(nameOfFn,getIOObjFn);
void setDBMan_DBKey_Fns( nameOfFn nfn, getIOObjFn ifn )
{
    nameoffn_ = nfn;
    getioobjfn_ = ifn;
}

BufferString nameOf( const DBKey& dbky )
{
    return (*nameoffn_)( dbky );
}

IOObj* getIOObj( const DBKey& dbky )
{
    return (*getioobjfn_)( dbky );
}


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

    char* ptrbq = inpstr.find( '`' );
    if ( ptrbq )
	*ptrbq = '\0';

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


DBKey* DBKey::getFromString( const char* str )
{
    FullDBKey* fdbky = new FullDBKey;
    fdbky->fromString( str );
    if ( !fdbky->isInCurrentSurvey() )
	return fdbky;

    DBKey* dbky = new DBKey( *fdbky );
    delete fdbky;
    return dbky;
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



DBKey DBKey::getFromStr( const char* str )
{
    DBKey ret;
    ret.fromString( str );
    return ret;
}


DBKey DBKey::getFromI64( od_int64 i64 )
{
    GroupedID id = GroupedID::getInvalid();
    id.fromInt64( i64 );
    return DBKey( id.groupNr(), id.objNr() );
}


uiString DBKey::toUiString() const
{
    return ::toUiString( toString() );
}


bool DBKeySet::operator ==( const DBKeySet& oth ) const
{
    const size_type sz = size();
    if ( sz != oth.size() )
	return false;

    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( *dbkys_.get(idx) != *oth.dbkys_.get(idx) )
	    return false;
    }
    return true;
}


DBKeySet::idx_type DBKeySet::indexOf( const DBKey& dbky ) const
{
    const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( *dbkys_.get(idx) == dbky )
	    return idx;
    }
    return -1;
}


bool DBKeySet::addIfNew( const DBKey& dbky )
{
    if ( isPresent(dbky) )
	return false;

    add( dbky );
    return true;
}


void DBKeySet::append( const DBKeySet& oth, bool allowduplicates )
{
    if ( allowduplicates )
	deepAppendClone( dbkys_, oth.dbkys_ );
    else
    {
	for ( auto dbky : oth )
	    addIfNew( *dbky );
    }
}


void DBKeySet::insert( idx_type idx, const DBKey& dbky )
{
    dbkys_.insertAt( dbky.clone(), idx );
}


DBKeySet& DBKeySet::removeSingle( idx_type idx )
{
    delete dbkys_.removeSingle( idx );
    return *this;
}


DBKeySet& DBKeySet::removeRange( idx_type idx1, idx_type idx2 )
{
    if ( idx1 == idx2 )
	return removeSingle( idx1 );

    if ( idx1 > idx2 )
	std::swap( idx1, idx2 );
    const size_type sz = size();
    if ( idx1 >= sz )
	return *this;
    if ( idx2 >= sz-1 )
	idx2 = sz-1;

    for ( idx_type idx=idx1; idx<=idx2; idx++ )
	delete dbkys_.get( idx );

    dbkys_.removeRange( idx1, idx2 );
    return *this;
}


DBKeySet& DBKeySet::remove( const DBKey& dbky )
{
    const idx_type idx = indexOf( dbky );
    if ( idx >= 0 )
	removeSingle( idx );
    return *this;
}


void DBKeySet::addTo( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<size(); idx++ )
	bss.add( ((*this)[idx]).toString() );
}



FullDBKey& FullDBKey::operator =( const FullDBKey& oth )
{
    if ( this != &oth )
    {
	DBKey::operator =( oth );
	survloc_ = oth.survloc_;
    }
    return *this;
}


FullDBKey& FullDBKey::operator =( const DBKey& dbky )
{
    if ( this != &dbky )
    {
	mDynamicCastGet( const FullDBKey*, fdbky, &dbky )
	if ( fdbky )
	    return operator =( *fdbky );

	DBKey::operator =( dbky );
	if ( !survloc_.isCurrentSurvey() )
	    survloc_.setCurrentSurvey();
    }
    return *this;
}


bool FullDBKey::operator ==( const FullDBKey& oth ) const
{
    return survloc_ == oth.survloc_
	&& DBKey::operator ==( oth );
}


bool FullDBKey::operator !=( const FullDBKey& oth ) const
{
    return !(*this == oth);
}


bool FullDBKey::operator ==( const DBKey& dbky ) const
{
    mDynamicCastGet( const FullDBKey*, fdbky, &dbky )
    if ( fdbky )
	return operator ==( *fdbky );

    return survloc_ == SurveyDiskLocation()
	&& DBKey::operator ==( dbky );
}


bool FullDBKey::operator !=( const DBKey& dbky ) const
{
    return !(*this == dbky);
}


BufferString FullDBKey::toString() const
{
    BufferString ret = DBKey::toString();
    ret.add( "`" ).add( survloc_.fullPath() );
    return ret;
}


void FullDBKey::fromString( const char* str )
{
    DBKey::fromString( str );

    FixedString inpstr( str );
    const char* ptrbq = inpstr.find( '`' );
    if ( ptrbq )
    {
	const File::Path fp( ptrbq + 1 );
	survloc_.set( fp );
    }
}


BufferString FullDBKey::surveyName() const
{
    return survloc_.surveyName();
}
