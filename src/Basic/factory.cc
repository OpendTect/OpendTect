/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2009
________________________________________________________________________

-*/


#include "factory.h"

#include "separstr.h"


const char* FactoryBase::key( int idx ) const
{
    return keys_.validIdx(idx) ? keys_.get(idx).str() : 0;
}


uiString FactoryBase::userName( int idx ) const
{
    return usernames_.validIdx(idx) ? usernames_.get(idx)
				    : toUiString( key(idx) );
}



bool FactoryBase::getKeyAndAliases( const char* inpky,
				    BufferString& ky, BufferString& aliases )
{
    FileMultiString fms( inpky );
    const int sz = fms.size();
    if ( sz < 1 )
	{ pFreeFnErrMsg("Empty key"); return false; }

    ky = fms[0];
    FileMultiString curaliases;
    BufferStringSet knownkeys;
    knownkeys.add( ky );
    for ( int idx=1; idx<sz;  idx++ )
    {
	const BufferString aliasky = fms[idx];
	if ( !aliasky.isEmpty() && !knownkeys.isPresent(aliasky) )
	{
	    curaliases += aliasky;
	    knownkeys.add( aliasky );
	}
    }

    aliases.set( curaliases.str() );
    return true;
}


void FactoryBase::addNames( const char* ky, const uiString& username )
{
    BufferString thiskey, aliases;
	if (!getKeyAndAliases(ky, thiskey, aliases))
	return;

	const int idx = keys_.indexOf(thiskey);
    if ( idx > 0 )
	setNames( idx, ky, username );
    else
    {
		keys_.add(thiskey);
	aliases_.add( aliases );
	usernames_.add(username.isEmpty() ? toUiString(thiskey) : username);
    }
}


void FactoryBase::setNames( int idx, const char* ky,
			    const uiString& username )
{
    BufferString thiskey, aliases;
    if ( !getKeyAndAliases(ky,thiskey,aliases) )
	return;

    keys_.get( idx ) = thiskey;
    aliases_.get( idx ) = aliases;
    usernames_.get( idx ) = username.isEmpty() ? toUiString(thiskey) : username;
}


void FactoryBase::setDefaultIdx( int idx )
{
    if ( keys_.validIdx(idx) )
	defaultkeyidx_ = idx;
}


void FactoryBase::setDefaultKey( const char* ky )
{
    setDefaultIdx( indexOf(ky) );
}


const char* FactoryBase::defaultKey() const
{
    if ( keys_.validIdx(defaultkeyidx_) )
	return keys_.get(defaultkeyidx_).str();
    return keys_.isEmpty() ? 0 : keys_.get(0).str();
}


int FactoryBase::indexOf( const char* ky ) const
{
    if ( !ky || !*ky )
	return -1;

    int ret = keys_.indexOf( ky );
    if ( ret >= 0 )
	return ret;

    for ( int idx=0; idx<aliases_.size(); idx++ )
    {
	FileMultiString fms( aliases_.get(idx) );
	ret = fms.indexOf( ky );
	if ( ret >= 0 )
	    return ret;
    }

    return -1;
}


const char* FactoryBase::keyOfLastCreated() const
{
    const int idx = lastcreatedidx_;
    return keys_.validIdx(idx) ? keys_.get(idx).str() : 0;
}
