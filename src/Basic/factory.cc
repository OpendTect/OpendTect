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
    names_.add( sep[0] );
    aliases_.add( name );
    usernames_.add(
	!username.isEmpty() ? username : toUiString(sep[0]));
}


void FactoryBase::setNames( int idx, const char* name,
			    const uiString& username )
{
    SeparString sep( name, cSeparator() );
    (*names_[idx]) = sep[0];
    (*aliases_[idx]) = name;
    usernames_[idx] =
	!username.isEmpty() ? username : toUiString(sep[0]);
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
