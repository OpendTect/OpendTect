/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.10 2003-10-19 13:53:07 bert Exp $
________________________________________________________________________

-*/

#include "hostdata.h"
#include "strmdata.h"
#include "strmprov.h"
#include "ascstream.h"
#include "errh.h"
#include <unistd.h>
#ifdef __win__
# include <windows.h>
#else
# include <netdb.h>
#endif


const char* HostData::localHostName()
{
    static char ret[256];   
    gethostname( ret, 256 );
    return ret;
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
    	: realaliases_(false)
    	, rshcomm_("rsh")
    	, defnicelvl_(19)
    	, portnr_(1963)
{
    const char* bhfnm = "BatchHosts";
    if ( getenv("DTECT_BATCH_HOSTS_FILENAME") )
	bhfnm = getenv("DTECT_BATCH_HOSTS_FILENAME");
    BufferString fname( GetDataFileName(bhfnm) );

#ifdef __win__
    readHostFile(fname);
#else
    if ( !readHostFile(fname) )
    {
	sethostent(0);
	struct hostent* he;
	while ( (he = gethostent()) )
	{
	    HostData* newhd = new HostData( he->h_name );
	    char** al = he->h_aliases;
	    while ( *al )
	    {
		if ( !newhd->isKnownAs(*al) )
		    newhd->aliases_ += new BufferString(*al);
		al++;
	    }
	    *this += newhd;
	}
	endhostent();
	realaliases_ = true;
    }
#endif
    handleLocal();
}


bool HostDataList::readHostFile( const char* fname )
{
    StreamData sd = StreamProvider( fname ).makeIStream();
    if ( !sd.usable() || !sd.istrm->good() )
	{ sd.close(); return false; }

    ascistream astrm( *sd.istrm );
    if ( !astrm.isOfFileType("Batch Processing Hosts") )
    {
	BufferString msg( fname );
	msg += ": invalid hosts file (invalid file header)";
	ErrMsg( msg ); sd.close(); return false;
    }

    if ( atEndOfSection(astrm) ) astrm.next();
    while ( !atEndOfSection(astrm) )
    {
	if ( astrm.hasKeyword("Remote shell") )
	    rshcomm_ = astrm.value();
	if ( astrm.hasKeyword("Default nice level") )
	    defnicelvl_ = astrm.getVal();
	if ( astrm.hasKeyword("First port") )
	    portnr_ = astrm.getVal();
	astrm.next();
    }
    while ( !atEndOfSection(astrm.next()) )
    {
	HostData* newhd = new HostData( astrm.keyWord() );
	if ( *astrm.value() )
	    newhd->aliases_ += new BufferString( astrm.value() );
	*this += newhd;
    }

    sd.close();
    return true;
}


void HostDataList::handleLocal()
{
    int sz = size();
    if ( !sz ) return;

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

    sz = size();
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
