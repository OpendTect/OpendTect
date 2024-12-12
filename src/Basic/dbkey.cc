/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "dbkey.h"

#include "bufstringset.h"
#include "filepath.h"
#include "ptrman.h"
#include "surveydisklocation.h"
#include "typeset.h"


DBKey::DBKey( const MultiID& mid, const SurveyDiskLocation& sdl )
    : MultiID(mid)
{
    if ( mid.groupID() >= 0 )
	survloc_ = new SurveyDiskLocation( sdl );
    else
	survloc_ = new SurveyDiskLocation( nullptr, sdl.basePath() );
}


DBKey::DBKey( const DBKey& key )
{
    *this = key;
}


DBKey::~DBKey()
{
    delete survloc_;
}


DBKey& DBKey::operator =( const DBKey& oth )
{
    if ( &oth == this )
	return *this;

    MultiID::operator =( oth );
    delete survloc_;
    survloc_ = oth.survloc_ ? new SurveyDiskLocation(*oth.survloc_) : nullptr;

    return *this;
}


bool DBKey::operator ==( const DBKey& oth ) const
{
    if ( &oth == this )
	return true;

    if ( MultiID::operator!=(oth) )
	return false;

    const bool iscursuv = isInCurrentSurvey();
    const bool othiscursurv = oth.isInCurrentSurvey();
    if ( iscursuv && othiscursurv )
	return true;
    else if ( iscursuv || othiscursurv )
	return false;
    else if ( *survloc_ != *oth.survloc_ )
	return false;

    return true;
}


bool DBKey::operator !=( const DBKey& oth ) const
{
    return !(*this == oth);
}


void DBKey::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    deleteAndNullPtr( survloc_ );
    if ( !sdl.isCurrentSurvey() )
    {
	survloc_ = new SurveyDiskLocation;
	*survloc_ = sdl;
    }
}


const SurveyDiskLocation& DBKey::surveyDiskLocation() const
{
    static const SurveyDiskLocation emptysdl_( 0, 0 );
    return survloc_ ? *survloc_ : emptysdl_;
}


void DBKey::clearSurveyDiskLocation()
{
    deleteAndNullPtr( survloc_ );
}


bool DBKey::isInCurrentSurvey() const
{
    return !survloc_ || survloc_->isCurrentSurvey();
}


BufferString DBKey::toString( bool withsurvloc ) const
{
    BufferString ret = MultiID::toString();
    if ( withsurvloc && survloc_ )
	ret.add( "`" ).add( surveyDiskLocation().fullPath() );

    return ret;
}


bool DBKey::fromString( const char* str )
{
    setUdf();
    deleteAndNullPtr( survloc_ );

    BufferString keystr( str );
    if ( keystr.isEmpty() )
	return true;

    char* ptr = keystr.find( '`' );
    if ( ptr )
    {
	*ptr = '\0';
	const BufferString survpath = ptr + 1;
	const FilePath fp( survpath );
	const SurveyDiskLocation sdl( fp );
	setSurveyDiskLocation( sdl );
    }

    // check if keystr is of type MultiID.
    return MultiID::fromString( keystr );
}


const SurveyInfo& DBKey::surveyInfo() const
{
    return surveyDiskLocation().surveyInfo();
}


DBKey DBKey::getLocal() const
{
    DBKey dbkey = *this;
    dbkey.clearSurveyDiskLocation();
    return dbkey;
}


// DBKeySet
DBKeySet::DBKeySet()
{}


DBKeySet::DBKeySet( const DBKeySet& oth )
{
    deepAppend( dbkys_, oth.dbkys_ );
}


DBKeySet::DBKeySet( const TypeSet<MultiID>& mids )
{
    *this = mids;
}


DBKeySet::DBKeySet( const DBKey& dbky )
{
    add( dbky );
}


DBKeySet::~DBKeySet()
{
    deepErase(dbkys_);
}


DBKeySet* DBKeySet::clone() const
{
    return new DBKeySet(*this);
}


DBKeySet& DBKeySet::operator=( const DBKeySet& oth )
{
    deepCopy( dbkys_, oth.dbkys_ );
    return *this;
}


DBKeySet& DBKeySet::operator=( const TypeSet<MultiID>& mids )
{
    erase();
    for ( int idx=0; idx<mids.size(); idx++ )
	add( mids[idx] );

    return *this;
}


bool DBKeySet::operator ==( const DBKeySet& oth ) const
{
    if ( &oth == this )
	return true;

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


bool DBKeySet::operator !=( const DBKeySet& oth ) const
{
    return !(*this == oth);
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
	deepAppend( dbkys_, oth.dbkys_ );
    else
    {
	for ( auto dbky : oth )
	    addIfNew( *dbky );
    }
}


void DBKeySet::insert( idx_type idx, const DBKey& dbky )
{
    dbkys_.insertAt( new DBKey(dbky), idx );
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
	bss.add( get(idx).toString(true) );
}
