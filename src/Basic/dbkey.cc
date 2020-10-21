/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "dbkey.h"

#include "bufstringset.h"
#include "surveydisklocation.h"
#include "typeset.h"


DBKey::~DBKey()
{}


void DBKey::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    delete survloc_;
    if ( sdl.isCurrentSurvey() )
        survloc_ = nullptr;
    else
        survloc_ = new SurveyDiskLocation( sdl );
}


const SurveyDiskLocation& DBKey::surveyDiskLocation() const
{
    static const SurveyDiskLocation emptysdl_( 0, 0 );
    return survloc_ ? *survloc_ : emptysdl_;
}


void DBKey::clearSurveyDiskLocation()
{
    delete survloc_;
    survloc_ = nullptr;
}


bool DBKey::isInCurrentSurvey() const
{
    return !survloc_ || survloc_->isCurrentSurvey();
}


BufferString DBKey::toString( bool withsurvloc ) const
{
    BufferString ret = buf();
    if ( withsurvloc && survloc_ )
	ret.add( "`" ).add( surveyDiskLocation().fullPath() );

    return ret;
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

DBKeySet& DBKeySet::operator=( const TypeSet<MultiID>& mids )
{
    for ( int idx=0; idx<mids.size(); idx++ )
	add( mids[idx] );

    return *this;
}


bool DBKeySet::operator==( const DBKeySet& oth ) const
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
