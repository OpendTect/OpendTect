/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.2 2002-04-08 20:58:14 bert Exp $
________________________________________________________________________

-*/

#include "hostdata.h"
#include <netdb.h>
//WIN
#include <unistd.h>


const char* HostData::localHostName()
{
    static char ret[256];   
//WIN
    gethostname( ret, 256 );
    return ret;
}


const char* HostData::shortestName() const
{
    int len = name_.size();
    int ishortest = -1;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( aliases_[idx]->size() < len )
	    { len = aliases_[idx]->size(); ishortest = idx; }

    return (const char*)(ishortest < 0 ? name_ : *aliases_[ishortest]);
}


bool HostData::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm ) return false;
    if ( name_ == nm ) return true;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return true;
    return false;
}


void HostData::addAlias( const char* nm )
{
    if ( !nm || !*nm || name_ == nm ) return;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( *aliases_[idx] == nm ) return;
    aliases_ += new BufferString( nm );
}


HostDataList::HostDataList()
{
//WIN
    sethostent(0);
    struct hostent* he;
    while ( (he = gethostent()) )
    {
	HostData* newhd = new HostData( he->h_name );
	char** al = he->h_aliases;
	while ( *al )
	    { newhd->aliases_ += new BufferString(*al); al++; }
	*this += newhd;
    }
    endhostent();

    handleLocal();
}


void HostDataList::handleLocal()
{
    const int sz = size();
    if ( !sz ) return;

//WIN
    const char* localhoststd = "localhost";

    bool havelocalhost = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];
	if ( hd->isKnownAs(localhoststd)
	  || hd->isKnownAs("localhost.localdomain") )
	{
	    // Ensure this is the first entry
	    if ( idx != 0 )
	    {
		*this -= hd;
		insertAt( hd, 0 );
	    }
	    havelocalhost = true;
	    break;
	}
    }

    BufferString hnm( HostData::localHostName() );
    if ( hnm == "" ) return;
    if ( !havelocalhost )
    {
	HostData* hd = new HostData( hnm );
	hd->addAlias( localhoststd );
	insertAt( hd, 0 );
    }

    HostData& lochd = *(*this)[0];
    if ( hnm != lochd.name() )
    {
	BufferString oldnm = lochd.name();
	lochd.name_ = hnm;
	lochd.addAlias( oldnm );
	for ( int idx=0; idx<lochd.aliases_.size(); idx++ )
	{
	    BufferString* al = lochd.aliases_[idx];
	    if ( *al == hnm )
	    {
		lochd.aliases_ -= al;
		delete al;
	    }
	}
    }

    for ( int idx=1; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];
	if ( hd->isKnownAs(hnm) )
	{
	    *this -= hd;
	    lochd.addAlias( hd->name() );
	    for ( int idx=0; idx<hd->aliases_.size(); idx++ )
		lochd.addAlias( *hd->aliases_[idx] );
	    delete hd;
	}
    }
}
