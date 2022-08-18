/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "repos.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "safefileio.h"
#include "file.h"
#include <ctype.h>


bool Repos::isUserDefined( Repos::Source src )
{
    return src == Temp || src > ApplSetup;
}


uiString Repos::descriptionOf( Repos::Source src )
{
    const char* fn = "Repos::descriptionOf";
    switch ( src )
    {
	case Rel:	return od_static_tr( fn, "OpendTect Release" );
	case ApplSetup:	return od_static_tr( fn, "Common Application Setup" );
	case Data:	return od_static_tr( fn, "Survey Data Root" );
	case Survey:	return od_static_tr( fn, "Current Survey" );
	case User:	return od_static_tr( fn, "Your Home/User directory" );
	default:	return od_static_tr( fn, "Temporary Storage" );
    }
}


bool Repos::FileProvider::next( Repos::Source& src, bool rev )
{
    if ( rev )
    {
	if ( src == Rel )
	    return false;
	else if ( src == Temp )
	    src = User;
	else
	    src = (Source)(((int)src)-1);;
    }
    else
    {
	if ( src == User )
	    return false;
	else
	    src = (Source)(((int)src)+1);;
    }
    return true;
}


void Repos::FileProvider::getFname( BufferString& res, bool withdot ) const
{
    res = withdot ? "." : "";
    res += basenm_;
    res.toLower();
}


BufferString Repos::FileProvider::fileName( Repos::Source src ) const
{
    BufferString ret;

#define mSetRet(fn,yn) \
	getFname( ret, yn ); \
	ret = FilePath( fn(), ret ).fullPath()

    switch ( src )
    {
    case Repos::Temp: {
	ret = FilePath( FilePath::getTempDir(), basenm_ ).fullPath();
    } break;
    case Repos::Survey: {
	mSetRet(GetDataDir,true);
    } break;
    case Repos::Data: {
	mSetRet(GetBaseDataDir,true);
    } break;
    case Repos::User: {
	mSetRet(GetSettingsDir,false);
    } break;
    case Repos::ApplSetup: {
	ret = GetSetupDataFileName( ODSetupLoc_ApplSetupOnly, basenm_, 0 );
    } break;
    case Repos::Rel: {
	ret = GetSetupDataFileName( ODSetupLoc_SWDirOnly, basenm_, 0 );
    } break;
    }

    return ret;
}


bool Repos::FileProvider::removeFile( Source src )
{
    SafeFileIO sfio( fileName(src) );
    return sfio.remove();
}



Repos::IOParSet::IOParSet( const char* basenm )
    : basenm_(basenm)
{
    FileProvider rfp( basenm_ );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );

	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	while ( sfio.istrm().isOK() )
	{
	    IOPar* par = new IOPar( rfp.source() );
	    par->getFrom( astrm );
	    if ( par->isEmpty() )
		{ delete par; continue; }

	    add( par );
	}
	sfio.closeSuccess();
    }
}


Repos::IOParSet& Repos::IOParSet::doAdd( Repos::IOPar* iop )
{
    if ( !iop )
	return *this;

    const int paridx = find( iop->name() );
    if ( paridx < 0 )
	ObjectSet<IOPar>::doAdd( iop );
    else
	replace( paridx, iop );

    return *this;
}


ObjectSet<const Repos::IOPar> Repos::IOParSet::getEntries(
					Repos::Source src ) const
{
    ObjectSet<const IOPar> ret;
    for ( int idx=0; idx<size(); idx++ )
    {
	const IOPar* entry = (*this)[idx];
	if ( entry->src_ == src )
	    ret += entry;
    }
    return ret;
}


int Repos::IOParSet::find( const char* nm ) const
{
    int ret = -1;
    for ( int idx=0; idx<size(); idx++ )
	if ( ((*this)[idx])->name() == nm )
	    { ret = idx; break; }
    return ret;
}


bool Repos::IOParSet::write( Repos::Source reqsrc ) const
{
    return write( &reqsrc );
}


bool Repos::IOParSet::write( const Repos::Source* reqsrc ) const
{
    bool rv = true;

    FileProvider rfp( basenm_ );
    while ( rfp.next() )
    {
	const Source cursrc = rfp.source();
	if ( reqsrc && *reqsrc != cursrc )
	    continue;

	ObjectSet<const IOPar> srcentries = getEntries( cursrc );
	const BufferString fnm( rfp.fileName() );
	if ( File::exists(fnm) && !File::isWritable(fnm) )
	    { rv = false; continue; }

	SafeFileIO sfio( fnm );
	if ( !sfio.open(false) )
	    { rv = false; continue; }

	ascostream astrm( sfio.ostrm() );
	astrm.putHeader( basenm_ );
	for ( int idx=0; idx<srcentries.size(); idx++ )
	    srcentries[idx]->putTo( astrm );

	if ( sfio.ostrm().isOK() )
	    sfio.closeSuccess();
	else
	    { rv = false; sfio.closeFail(); }
    }

    return rv;
}


BufferString Repos::IOParSet::fileName( Source src ) const
{
    FileProvider rfp( basenm_ );
    return rfp.fileName( src );
}
