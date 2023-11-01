/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "factory.h"

#include "separstr.h"


FactoryBase::~FactoryBase()
{}


int FactoryBase::size() const
{ return names_.size(); }

bool FactoryBase::isEmpty() const
{ return names_.isEmpty(); }

const char* FactoryBase::currentName() const
{ return currentname_.getObject().str(); }

BufferString& FactoryBase::errMsg() const
{ return currentname_.getObject(); }


void FactoryBase::addNames( const char* name, const uiString& username )
{
    SeparString sep( name, cSeparator() );
    names_.add( (const char*) sep[0] );
    aliases_.add( name );
    usernames_.add(
	!username.isEmpty() ? username : toUiString((const char*) sep[0]));
}


void FactoryBase::setNames( int idx, const char* name,
			    const uiString& username )
{
    SeparString sep( name, cSeparator() );
    (*names_[idx]) = (const char*) sep[0];
    (*aliases_[idx]) = name;
    usernames_[idx] =
	!username.isEmpty() ? username : toUiString((const char*) sep[0]);
}


const BufferStringSet& FactoryBase::getNames() const
{ return names_; }

const uiStringSet& FactoryBase::getUserNames() const
{ return usernames_; }


void FactoryBase::setDefaultName( int idx )
{
    if ( idx<0 || idx>=names_.size() || !names_[idx] )
	defaultname_.empty();
    else
    {
	SeparString sep( names_[idx]->buf(), cSeparator() );
	defaultname_ = sep[0];
    }
}


const char* FactoryBase::getDefaultName() const
{ return defaultname_.isEmpty() ? 0 : defaultname_.buf(); }


int FactoryBase::indexOf( const char* name ) const
{
    if ( !name )
	return -1;

    SeparString sep( 0, cSeparator() );
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( !aliases_[idx] )
	    continue;
	sep = aliases_[idx]->buf();
	if ( sep.indexOf( name )!=-1 )
	    return idx;
    }

    return -1;
}


void FactoryBase::removeByIndex( int idx )
{
    if ( !names_.validIdx(idx) )
	return;

    names_.removeSingle( idx );
    usernames_.removeSingle( idx );
    aliases_.removeSingle( idx );
}


bool FactoryBase::moveAfter_( int from, int to )
{
    if ( !names_.validIdx(from) || !names_.validIdx(to) ||
	 to == names_.size()-2 )
	return false;

    if ( from < to )
    {
	int idx = from;
	while( idx < to )
	{
	    names_.swap( idx, idx+1 );
	    usernames_.swap( idx, idx+1 );
	    aliases_.swap( idx, idx+1 );
	    idx++;
	}
    }
    else
    {
	int idx = from-1;
	while ( idx > to && idx > 0 )
	{
	    names_.swap( idx, idx+1 );
	    usernames_.swap( idx, idx+1 );
	    aliases_.swap( idx, idx+1 );
	    idx--;
	}
    }

    return true;
}
