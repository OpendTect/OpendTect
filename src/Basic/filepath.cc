/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: filepath.cc,v 1.13 2005-11-16 14:58:09 cvsarend Exp $";

#include "filepath.h"
#include "envvars.h"
#include <iostream>

#include "winutils.h"

const char* FilePath::sPrefSep = ":";

const char* FilePath::dirSep( Style stl )
{
    static const char* wds = "\\";
    static const char* uds = "/";

    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? wds : uds;
} 

FilePath& FilePath::set( const char* _fnm )
{
    lvls_.deepErase(); prefix_ = ""; isabs_ = false;
    if ( !_fnm ) return *this;

    BufferString __fnm ( _fnm );

    const char* fnm = __fnm.buf();

    skipLeadingBlanks( fnm );
    if ( !*fnm ) return *this;

    char* ptr = strchr( fnm, *sPrefSep );
    if ( ptr )
    {
	const char* dsptr = strchr( fnm, *dirSep(Windows) );
	if ( dsptr > ptr )
	{
	    prefix_ = fnm;
	    *strchr( prefix_.buf(), *sPrefSep ) = '\0';
	    fnm = ptr + 1;
	}
    }

    isabs_ = *fnm == '\\' || *fnm == '/';

    if ( isabs_ ) fnm++;
    addPart( fnm );
    compress();
    return *this;
}


void FilePath::addPart( const char* fnm )
{
    if ( !fnm ) return;

    skipLeadingBlanks( fnm );
    char prev = ' ';
    char buf[PATH_LENGTH];
    char* bufptr = buf;
    bool remdblsep = false;

    while ( *fnm )
    {
	char cur = *fnm;

	if ( cur != *dirSep(Local) )
	    remdblsep = true;
	else
	{
	    if ( (prev != *dirSep(Windows) && prev != *dirSep(Windows))
		    || !remdblsep )
	    {
		*bufptr = '\0';
		if ( buf[0] ) lvls_.add( buf );
		bufptr = buf;
		*bufptr = '\0';
	    }
	    fnm++;
	    continue;
	}

	*bufptr++ = cur;
	fnm++;
	prev = cur;
    }
    *bufptr = '\0';
    if ( buf[0] ) lvls_.add( buf );
}


void FilePath::compress( int startlvl )
{
    for ( int idx=startlvl; idx<lvls_.size(); idx++ )
    {
	const BufferString& bs = *lvls_[idx];
	int remoffs = 99999;
	if ( bs == "." )
	    remoffs = 0;
	else if ( bs == ".." && idx > 0 && *lvls_[idx-1] != ".." )
	    remoffs = 1;

	if ( idx-remoffs >= 0 )
	{
	    lvls_.remove( idx-remoffs, idx );
	    idx -= remoffs + 1;
	}
    }
}


FilePath& FilePath::insert( const char* fnm )
{
    if ( !fnm || !*fnm ) return *this;
    ObjectSet<BufferString> oldlvls;
    oldlvls.append( lvls_ );
    lvls_.ObjectSet<BufferString>::erase();
    set( fnm );
    lvls_.ObjectSet<BufferString>::append( oldlvls );
    return *this;
}


FilePath& FilePath::add( const char* fnm )
{
    if ( !fnm ) return *this;
    int sl = lvls_.size();
    addPart( fnm );
    compress( sl );
    return *this;
}


void FilePath::setFileName( const char* fnm )
{
    if ( !fnm || !*fnm )
    {
	if ( lvls_.size() )
	    lvls_.remove( lvls_.size()-1 );
    }
    else if ( !lvls_.size() )
	add( fnm );
    else
    {
	*lvls_[lvls_.size()-1] = fnm;
	compress( lvls_.size()-1 );
    }
}


void FilePath::setExtension( const char* ext, bool replace )
{
    if ( !ext ) ext = "";
    skipLeadingBlanks( ext );

    if ( *ext == '.' )
	ext++;
    if ( lvls_.size() < 1 )
    {
	if ( *ext )
	    add( ext );
	return;
    }

    BufferString& fname = *lvls_[lvls_.size()-1];
    char* ptr = strrchr( fname.buf(), '.' );
    if ( ptr && replace )
	strcpy( *ext ? ptr+1 : ptr, ext );
    else if ( *ext )
	{ fname += "."; fname += ext; }
}


const char* FilePath::extension() const
{
    if ( !lvls_.size() )
	return 0;

    const char* ret = strrchr( fileName().buf(), '.' );
    if ( ret ) ret++;
    return ret;
}


void FilePath::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ? lvls_.get(lvls_.size()-1) : 0 );
    set( pth );
    if ( fnm != "" )
	add( fnm );
}


BufferString FilePath::dirUpTo( int lvl ) const
{
    if ( lvl < 0 || lvl >= lvls_.size() )
	lvl = lvls_.size() - 1;

    BufferString ret;
    if ( *prefix_.buf() )
    {
	ret = prefix_;
	ret += sPrefSep;
    }
    if ( lvl < 0 )
	return ret;

    if ( isabs_ )
	ret += dirSep(Local);
    if ( lvls_.size() )
	ret += lvls_.get( 0 );

    for ( int idx=1; idx<=lvl; idx++ )
    {
	ret += dirSep(Local);
	ret += lvls_.get( idx );
    }

    return ret;
}


const BufferString& FilePath::dir( int nr ) const
{
    if ( nr < 0 || nr >= lvls_.size() )
	nr = lvls_.size()-1;
    return nr < 0 ? BufferString::empty() : *lvls_[nr];
}


BufferString FilePath::getTempName( const char* ext )
{
    BufferString fname;

#ifdef __win__

    if ( GetEnvVar("TMP") )
	fname = GetEnvVar( "TMP" );
    else if ( GetEnvVar("TEMP") )
	fname = GetEnvVar( "TEMP" );
    else // make sure we have at least write access...
    {
	fname = GetSpecialFolderLocation( CSIDL_PERSONAL );
	static bool warn = true;
	if ( warn )
	{
	    std::cerr << "WARNING: You don't have the TEMP or TMP environment "
		   "variable set.\nUsing '" << fname << "'." << std::endl;
	    warn = 0;
	}
    }

    fname += dirSep(Local); fname += "od";

#else

    fname = "/tmp/od";

#endif

    static int counter = 0;
    int time_stamp = time( (time_t*)0 ) + counter++;
    char uniquestr[80];
    sprintf( uniquestr, "%X%X", GetPID(), (int)time_stamp );
    fname += uniquestr;

    if ( ext && *ext )
    {
	fname += ".";
	fname += ext;
    }

    return fname;
}


BufferString FilePath::mkCleanPath(const char* path, Style stl)
{
    if ( stl == Local )		stl = __iswin__ ? Windows : Unix;

    BufferString ret;
    if ( stl == Windows )	ret = getCleanWinPath( path ) ;
    else			ret = getCleanUnxPath( path ) ;

    return ret;
}
