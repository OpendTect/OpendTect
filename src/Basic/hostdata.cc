/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.19 2004-11-02 16:05:21 arend Exp $
________________________________________________________________________

-*/

#include "hostdata.h"
#include "strmdata.h"
#include "strmprov.h"
#include "ascstream.h"
#include "errh.h"
#include "separstr.h"
#include "filepath.h"
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


#define mTolower(bs) \
    { \
	char* ptr=bs.buf(); \
	for ( int idx=0; idx<bs.size(); idx++ ) \
	    { *ptr++ = tolower(*ptr); } \
    }

static FilePath getReplacePrefix( const FilePath& dir_,
		const FilePath& fromprefix_, const FilePath& toprefix_ )
{
    if ( !fromprefix_.nrLevels() || !toprefix_.nrLevels() )
	return FilePath(dir_);

    // convert paths to Unix style
    BufferString dir = dir_.fullPathStyled( FilePath::Unix );
    BufferString fromprefix = fromprefix_.fullPathStyled( FilePath::Unix );
    BufferString toprefix = toprefix_.fullPathStyled( FilePath::Unix );

    const char* tail = strstr( dir, fromprefix );
    if ( !tail )
    {
	BufferString frompreflow( fromprefix );
	mTolower( frompreflow );
	tail = strstr( dir, frompreflow );
    }

    if ( !tail ) return FilePath(dir_);
    tail += strlen( fromprefix );

    BufferString ret = toprefix;
    ret += tail;

    return FilePath(ret);
}

FilePath HostData::convPath( PathType pt, const FilePath& fp,
			     const HostData* from ) const
{
    if ( !from ) from = &localHost();
    return getReplacePrefix( fp, from->prefixFilePath(pt), prefixFilePath(pt) );
}

HostDataList::HostDataList( bool readhostfile )
    	: realaliases_(false)
    	, rshcomm_("rsh")
    	, defnicelvl_(19)
    	, portnr_(1963)
{
    const char* bhfnm = "BatchHosts";
    if ( getenv("DTECT_BATCH_HOSTS_FILENAME") )
	bhfnm = getenv("DTECT_BATCH_HOSTS_FILENAME");

    BufferString fname( SearchODFile(bhfnm) );

    if ( getenv("DTECT_BATCH_HOSTS_FILEPATH") )
	fname = getenv("DTECT_BATCH_HOSTS_FILEPATH");

    if ( readhostfile )
    {
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
    }
    handleLocal();
}


HostDataList& TheHDL()
{
    static HostDataList* thedatalist = 0;
    if ( !thedatalist ) thedatalist = new HostDataList();

    return *thedatalist;
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
	    win_data_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx data prefix") )
	    unx_data_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Win appl prefix") )
	    win_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx appl prefix") )
	    unx_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Data host") )
	    datahost_ = astrm.value();
	if ( astrm.hasKeyword("Data drive") )
	    datadrive_ = astrm.value();
	if ( astrm.hasKeyword("Data share") )
	    datashare_ = astrm.value();
	if ( astrm.hasKeyword("Password") )
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
		newhd->data_pr_ = (char*)val[2];
	    else 
		newhd->data_pr_ = newhd->iswin_ ? win_data_pr_ : unx_data_pr_;

	    if ( val[3] && *val[3] )
		newhd->appl_pr_ = (char*)val[3];
	    else 
		newhd->appl_pr_ = newhd->iswin_ ? win_appl_pr_ : unx_appl_pr_;

/* Datahost kan niet als string gezet worden als de host waar het over
 * gaat nog niet aangemaakt is ...
	    
	    if ( val[4] && *val[4] )
		newhd->datahost_ = (char*)val[4];
	    else if ( newhd->iswin_ )
		newhd->datahost_ = datahost_;

	    if ( val[5] && *val[5] )
		newhd->datadrive_ = (char*)val[5];
	    else if ( newhd->iswin_ )
		newhd->datadrive_ = datadrive_;

	    if ( val[6] && *val[6] )
		newhd->datashare_ = (char*)val[6];
	    else if ( newhd->iswin_ )
		newhd->datashare_ = datashare_;
	    
	    if ( val[7] && *val[7] )
		newhd->pass_ = (char*)val[7];
	    else if ( newhd->iswin_ )
		newhd->pass_ = pass_;
*/
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

    HostData* localhd = 0;
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
	    localhd = hd;
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
	    localhd = hd;
	    break;
	}
    }

    BufferString hnm( HostData::localHostName() );
    if ( hnm == "" ) return;
    if ( !localhd )
    {
#ifdef __win__
	localhd = new HostData( hnm, true ); // true: bool isWin()
#else
	localhd = new HostData( hnm, false );
#endif
	localhd->addAlias( localhoststd );
	insertAt( localhd, 0 );
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
	hd->setLocalHost( *localhd );

	if ( hd->isKnownAs(hnm) )
	{
	    *this -= hd;
	    lochd.addAlias( hd->name() );
	    for ( int idx=0; idx<hd->aliases_.size(); idx++ )
		lochd.addAlias( *hd->aliases_[idx] );
	    delete hd;
	}
    }

    if( datahost_ != "" )
    {
	HostData* dh = 0; 
	int sz = size();

	for ( int idx=0; idx<sz; idx++ )
	{
	    HostData* hd = (*this)[idx];

	    if ( hd->isKnownAs(datahost_) )
	    {
		dh = hd;
		break;
	    }
	} 

	if ( dh )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		(*this)[idx]->datahost_ = dh;
	    }
	}
    }
}
