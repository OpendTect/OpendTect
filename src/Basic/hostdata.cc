/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.17 2004-10-21 14:53:24 dgb Exp $
________________________________________________________________________

-*/

#include "hostdata.h"
#include "strmdata.h"
#include "strmprov.h"
#include "ascstream.h"
#include "errh.h"
#include "separstr.h"
#include <iostream>
# include <unistd.h>
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

    BufferString fname( SearchConfigFile(bhfnm) );

    if ( getenv("DTECT_BATCH_HOSTS_FILEPATH") )
	fname = getenv("DTECT_BATCH_HOSTS_FILEPATH");

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

static const char* drvstr="/cygdrive/";

const char* getCleanUnxPath( const char* path )
{
    static BufferString buf; buf =path;
    char* ptr = buf.buf();
    skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );
    replaceCharacter( ptr, '\\' , '/' );
    replaceCharacter( ptr, ';' , ':' );

    char* drivesep = strstr( ptr, ":" );
    if ( !drivesep ) return ptr;
    *drivesep = '\0';

    static BufferString res;
    res = drvstr;
    *ptr = tolower(*ptr);
    res += ptr;
    res += ++drivesep;

    return res;
}

static const char* getCleanWinPath( const char* path )
{
    static BufferString ret; ret = path;

    BufferString buf(path);
    char* ptr = buf.buf();

    skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );

    char* cygdrv = strstr( ptr, drvstr );
    if( cygdrv )
    {
	char* drv = cygdrv + strlen( drvstr );
	char* buffer = ret.buf();

	*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
	ret += ++drv;

    }

    replaceCharacter( ret.buf(), '/', '\\' );

    return ret;
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
	if ( astrm.hasKeyword("Win data prefix") )
	    win_data_prefix_ = getCleanUnxPath( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx data prefix") )
	    unx_data_prefix_ = getCleanUnxPath( (char*) astrm.value() );
	if ( astrm.hasKeyword("Win appl prefix") )
	    win_appl_prefix_ = getCleanUnxPath( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx appl prefix") )
	    unx_appl_prefix_ = getCleanUnxPath( (char*) astrm.value() );
	if ( astrm.hasKeyword("Data host") )
	    datahost_ = astrm.value();
	if ( astrm.hasKeyword("Data drive") )
	    datadrive_ = astrm.value();
	if ( astrm.hasKeyword("Data share") )
	    datashare_ = astrm.value();
	if ( astrm.hasKeyword("Remote pass") )
	    remotepass_ = astrm.value();

	astrm.next();
    }
    while ( !atEndOfSection(astrm.next()) )
    {
	HostData* newhd = new HostData( astrm.keyWord() );
	if ( *astrm.value() )
	{
	    SeparString val( astrm.value(), ':' );

	    if ( val[0] && *val[0] )
		newhd->aliases_ += new BufferString( val[0] );

	    if ( val[1] && *val[1] )
	    {
		char* ptr = const_cast<char*>(val[1]);
		skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );
		newhd->iswin_ = !strcmp( ptr, "win" );
	    }

	    if ( val[2] && *val[2] )
		newhd->dataprefix_ = getCleanUnxPath( (char*)val[2] );

	    if ( val[3] && *val[3] )
		newhd->applprefix_ = getCleanUnxPath( (char*)val[3] );

	}
	*this += newhd;
    }

    sd.close();
    return true;
}


void HostDataList::handleLocal()
{
    int sz = size();

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
	else if ( hd->isKnownAs(HostData::localHostName()) )
	{
	    hd->addAlias( localhoststd );

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
#ifdef __win__
	HostData* hd = new HostData( hnm, true );
#else
	HostData* hd = new HostData( hnm, false );
#endif
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




#define mTolower(bs) \
    { \
	char* ptr=bs.buf(); \
	for ( int idx=0; idx<bs.size(); idx++ ) \
	    { *ptr++ = tolower(*ptr); } \
    }

static const char* getReplacePrefix( const char* dir_,
		const char* fromprefix_, const char* toprefix_, bool towin )
{
    if ( !fromprefix_ || !*fromprefix_ ) return dir_;
    if ( !toprefix_ || !*toprefix_ ) return dir_;

    static BufferString ret;
    static BufferString dir;

    dir = getCleanUnxPath( dir_ );
    BufferString fromprefix( getCleanUnxPath(fromprefix_) ); 
    BufferString toprefix( getCleanUnxPath(toprefix_) );

    const char* tail = strstr( dir, fromprefix );
    if ( !tail )
    {
	BufferString frompreflow( fromprefix );
	mTolower( frompreflow );
	tail = strstr( dir, frompreflow );
    }
    if ( !tail ) return dir;
    tail += strlen( fromprefix );

    ret = toprefix;
    ret += tail;

    if ( towin )
	return getCleanWinPath( ret.buf() );
    else
	replaceCharacter( ret.buf(), '\\' , '/' );

    return ret;
}




const char* HostDataList::getRemoteDataFileName( const char* fn,
				    const HostData& host, bool native )
{
    BufferString locprefix = dataPrefix( *(*this)[0] );
    BufferString remprefix = dataPrefix( host );
    BufferString appdir = getCleanUnxPath( fn );

    bool towin = host.isWin() && native;

    return getReplacePrefix( appdir, locprefix, remprefix, towin );
}

const char* HostDataList::getRemoteDataDir(  const HostData& host, bool native )
{
    return getRemoteDataFileName( GetDataDir(), host, native );
}


const char* HostDataList::getRemoteApplDir( const HostData& host, bool native )
{
    BufferString locprefix = applPrefix( *(*this)[0] );
    BufferString remprefix = applPrefix( host );
    BufferString appdir = getCleanUnxPath( GetSoftwareDir() );

    bool towin = host.isWin() && native;

    return getReplacePrefix( appdir, locprefix, remprefix, towin );
}


