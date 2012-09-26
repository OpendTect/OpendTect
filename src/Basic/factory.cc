/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "factory.h"

#include "separstr.h"


FactoryBase::~FactoryBase()
{}


void FactoryBase::addNames( const char* name, const char* username )
{
    SeparString sep( name, cSeparator() );
    names_.add( (const char*) sep[0] );
    aliases_.add( name );
    usernames_.add( username ? username : (const char*) sep[0] );
}


void FactoryBase::setNames( int idx, const char* name, const char* username )
{
    SeparString sep( name, cSeparator() );
    (*names_[idx]) = (const char*) sep[0];
    (*aliases_[idx]) = name;
    (*usernames_[idx]) = username ? username : (const char*) sep[0];
}


const BufferStringSet& FactoryBase::getNames( bool username ) const
{
    return username ? usernames_ : names_;
}


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
