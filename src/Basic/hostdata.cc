/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.23 2004-11-10 14:19:13 bert Exp $
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
    BufferString dir = dir_.fullPath( FilePath::Unix );
    BufferString fromprefix = fromprefix_.fullPath( FilePath::Unix );
    BufferString toprefix = toprefix_.fullPath( FilePath::Unix );

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

    BufferString sharehost;
    
    if ( atEndOfSection(astrm) ) astrm.next();
    while ( !atEndOfSection(astrm) )
    {
	if ( astrm.hasKeyword("Remote shell") )
	    rshcomm_ = astrm.value();
	if ( astrm.hasKeyword("Default nice level") )
	    defnicelvl_ = astrm.getVal();
	if ( astrm.hasKeyword("First port") )
	    portnr_ = astrm.getVal();
	if ( astrm.hasKeyword("Win appl prefix") )
	    win_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx appl prefix") )
	    unx_appl_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Win data prefix") )
	    win_data_pr_.set( (char*) astrm.value() );
	if ( astrm.hasKeyword("Unx data prefix") )
	    unx_data_pr_.set( (char*) astrm.value() );

	if ( astrm.hasKeyword("Data host") )
	    sharehost = astrm.value();  // store in a temporary
	if ( astrm.hasKeyword("Data drive") )
	    sharedata_.drive_ = astrm.value();
	if ( astrm.hasKeyword("Data share") )
	    sharedata_.share_ = astrm.value();
	if ( astrm.hasKeyword("Data pass") )
	    sharedata_.pass_ = astrm.value();

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

	    if ( val[4] && *val[4] )
		newhd->pass_ = (char*)val[4];
	    else if ( newhd->iswin_ )
		newhd->pass_ = sharedata_.pass_;

	    newhd->setShareData( &sharedata_ ); 

	}
	*this += newhd;
    }

    int sz = size(); 
    for ( int idx=0; idx<sz; idx++ )
    {
	HostData* hd = (*this)[idx];
	if ( hd->isKnownAs(sharehost) )
	{
	    sharedata_.host_ = hd;
	    break;
	}
    }
    static bool complain = true;
    if ( sharehost.size() && !sharedata_.host_ && complain )
    {
	BufferString msg("No host ");
	msg += sharehost;
	msg += "  found in ";
	msg += fname;
	msg += ". Multi machine batch processing might not work as expected.";

	ErrMsg( msg );
	complain = false;
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
}


HostData* HostDataList::findHost( const char* nm ) const
{
    const HostData* ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->isKnownAs(nm) )
	    { ret = (*this)[idx]; break; }
    }
    return const_cast<HostData*>( ret );
}
