/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/


#include "filepath.h"

#include "envvars.h"
#include "file.h"
#include "fixedstring.h"
#include "genc.h"
#include "msgh.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "winutils.h"
#include <time.h>
#include <string.h>

#include <QStorageInfo>

const char* FilePath::sPrefSep = ":";

static const FilePath::Style cOther = __iswin__ ? FilePath::Unix
						: FilePath::Windows;

FilePath::FilePath( const char* fnm )
{
    set( fnm );
}


FilePath::FilePath( const char* p1, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    set( p1 );
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


FilePath::FilePath( const FilePath& fp, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    *this = fp;
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


FilePath& FilePath::operator =( const FilePath& fp )
{
    lvls_ = fp.lvls_;
    prefix_ = fp.prefix_;
    isabs_ = fp.isabs_;
    return *this;
}


FilePath& FilePath::operator =( const char* fnm )
{ return (*this = FilePath(fnm)); }


bool FilePath::operator ==( const FilePath& fp ) const
{
    return lvls_ == fp.lvls_ && prefix_ == fp.prefix_ && isabs_ == fp.isabs_;
}


bool FilePath::operator ==( const char* fnm ) const
{ return *this == FilePath(fnm); }


bool FilePath::operator !=( const FilePath& fp ) const
{ return !(*this == fp); }


bool FilePath::operator != ( const char* fnm ) const
{ return !(*this == FilePath(fnm)); }


static bool isServerPath( const char* path )
{
    const FixedString pathstr = path;
    return pathstr.size()>1 && path[0]=='\\' && path[1]=='\\';
}


BufferString FilePath::getFullLongPath( const FilePath& fp )
{
#ifndef  __win__
    return fp.fullPath();
#else
    mDeclStaticString( longpath );
    longpath.setMinBufSize( 1025 );
    GetLongPathName(fp.fullPath(), longpath.getCStr(), longpath.minBufSize()-1);
    BufferString fpstr = longpath;
    if ( fpstr.isEmpty() )
	fpstr = fp.fullPath();
    return fpstr;
#endif // ! __win__
}


FilePath& FilePath::set( const char* inpfnm )
{
    lvls_.erase();
    prefix_.setEmpty();
    isabs_ = false;
    if ( !inpfnm || !*inpfnm )
	return *this;

    BufferString fnmbs( inpfnm );
    const char* fnm = fnmbs.buf();
    mSkipBlanks( fnm );
    if ( !*fnm )
	return *this;

    const char* ptr = firstOcc( fnm, *sPrefSep );
    if ( ptr )
    {
	const char* dsptr = firstOcc( fnm, *dirSep(Local) );
	const char* otherdsptr = firstOcc( fnm, *dirSep(cOther) );
	if ( otherdsptr && ( !dsptr || otherdsptr < dsptr ) )
	    dsptr = otherdsptr;

	if ( dsptr > ptr || (__iswin__ && !dsptr) )
	{
	    prefix_ = fnm;
	    *firstOcc( prefix_.getCStr(), *sPrefSep ) = '\0';
	    fnm = ptr + 1;
	}
    }
    else if ( isServerPath(fnm) )
    {
	prefix_ = fnm;
	char* prefixptr = prefix_.getCStr();
	prefixptr += 2;
	char* endptr = firstOcc( prefixptr, '\\' );
	if ( endptr )
	    *endptr = '\0';
	fnm += prefix_.size();
    }

    isabs_ = *fnm == '\\' || *fnm == '/' || (__iswin__ && !*fnm);
    if ( isabs_ && *fnm )
	fnm++;

    addPart( fnm );
    compress();
    return *this;
}


FilePath& FilePath::add( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    int sl = lvls_.size();
    addPart( fnm );
    compress( sl );

    return *this;
}


FilePath& FilePath::insert( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    BufferStringSet oldlvls( lvls_ );
    lvls_.setEmpty();
    set( fnm );
    lvls_.append( oldlvls );
    return *this;
}


void FilePath::setFileName( const char* fnm )
{
    if ( !fnm || !*fnm )
    {
	if ( lvls_.size() )
	    lvls_.removeSingle( lvls_.size()-1 );
    }
    else if ( lvls_.isEmpty() )
	add( fnm );
    else
    {
	*lvls_[lvls_.size()-1] = fnm;
	compress( lvls_.size()-1 );
    }
}


void FilePath::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ?
	    lvls_.get(lvls_.size()-1).buf() : (const char*) 0 );
    set( pth );
    if ( !fnm.isEmpty() )
	add( fnm );
}


void FilePath::setExtension( const char* ext, bool replace )
{
    if ( !ext ) ext = "";
    mSkipBlanks( ext );

    if ( *ext == '.' )
	ext++;
    if ( lvls_.size() < 1 )
    {
	if ( *ext )
	    add( ext );
	return;
    }

    BufferString& fname = *lvls_[lvls_.size()-1];
    char* ptr = lastOcc( fname.getCStr(), '.' );
    if ( ptr && replace )
    {
	*ptr = '\0';
	if ( *ext )
	    fname.add( "." ).add( ext );
    }
    else if ( *ext )
	{ fname.add( "." ).add( ext ); }
}


bool FilePath::exists() const
{
    return File::exists( fullPath() );
}


bool FilePath::isAbsolute() const
{ return isabs_; }


bool FilePath::isSubDirOf( const FilePath& b, FilePath* relpath ) const
{
    if ( b.isAbsolute()!=isAbsolute() )
	return false;

    if ( FixedString(b.prefix())!=prefix() )
	return false;

    const int nrblevels = b.nrLevels();
    if ( nrblevels>=nrLevels() )
	return false;

    for ( int idx=0; idx<nrblevels; idx++ )
    {
	if ( *lvls_[idx]!=*b.lvls_[idx] )
	    return false;
    }

    if ( relpath )
    {
	BufferString rel;
	for ( int idx=nrblevels; idx<nrLevels(); idx++ )
	{
	    if ( idx>nrblevels )
		rel += dirSep( Local );
	    rel += dir( idx );
	}

	relpath->set( rel.buf() );
    }

    return true;
}


bool FilePath::makeCanonical()
{
    BufferString fullpath = fullPath();
#ifndef __win__
    set( File::getCanonicalPath( fullpath.buf() ) );
#else
    BufferString winpath = File::getCanonicalPath( fullpath.buf() );
    winpath.replace( '/', '\\' );
    set( winpath );
#endif
    return true;
}


bool FilePath::makeRelativeTo( const FilePath&  b )
{
    const BufferString file = fullPath();
    const BufferString path = b.fullPath();
    set( File::getRelativePath( path.buf(), file.buf() ) );
    return true;
}



BufferString FilePath::fullPath( Style f, bool cleanup ) const
{
    BufferString res = dirUpTo( lvls_.size() );
    if ( cleanup )
	res = mkCleanPath( res, f );
    if ( isabs_ && ((__iswin__ && f==Local) || f==Windows) && nrLevels() < 1 )
	res.add( dirSep(Windows) );
    return res;
}


const char* FilePath::prefix() const
{ return prefix_.buf(); }


int FilePath::nrLevels() const
{ return lvls_.size(); }


const char* FilePath::extension() const
{
    if ( lvls_.isEmpty() )
	return 0;

    const char* ret = lastOcc( fileName().buf(), '.' );
    if ( ret ) ret++;
    return ret;
}


const OD::String& FilePath::fileName() const
{ return dir(-1); }


BufferString FilePath::baseName() const
{
    FilePath selfcopy( *this );
    selfcopy.setExtension( 0 );
    return selfcopy.fileName();
}


BufferString FilePath::getTimeStampFileName( const char* ext )
{
    BufferString tsfnm;
    BufferString datestr = Time::getDateTimeString();
    datestr.replace( ", ", "-" );
    datestr.replace( ':', '.' );
    datestr.replace( ' ', '_' );
    tsfnm += datestr.buf();
    tsfnm += ext;

    return tsfnm;
}


BufferString FilePath::pathOnly( Style f ) const
{
    BufferString res = dirUpTo( lvls_.size()-2 );
    if ( isabs_ && ((__iswin__ && f==Local) || f==Windows) && nrLevels() < 2 )
	res.add( dirSep(Windows) );

    return res;
}


const OD::String& FilePath::dir( int nr ) const
{
    if ( nr < 0 || nr >= lvls_.size() )
	nr = lvls_.size()-1;
    return nr < 0 ? BufferString::empty() : *lvls_[nr];
}


BufferString FilePath::dirUpTo( int lvl ) const
{
    if ( lvl >= lvls_.size() )
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


BufferString FilePath::fileFrom( int lvl ) const
{
    BufferString ret;

    const int sz = lvls_.size();
    for ( int ilvl=lvl; ilvl<sz; ilvl++ )
    {
	ret.add( lvls_.get( ilvl ) );
	if ( ilvl != sz-1 )
	    ret.add( dirSep(Local) );
    }

    return ret;
}


BufferString FilePath::getTempDir()
{
    BufferString tmpdir = File::getTempPath();
    if ( !File::exists(tmpdir) )
    {
	BufferString msg( "Temporary directory '", tmpdir, "'does not exist" );
	UsrMsg( msg );
    }
    else if ( !File::isWritable(tmpdir) )
    {
	BufferString msg( "Temporary directory '", tmpdir, "'is read-only" );
	UsrMsg( msg );
    }

    return tmpdir;
}


BufferString FilePath::getTempFileName( const char* typ, const char* ext )
{
    static Threads::Atomic<int> counter = 0;
    BufferString fname( "od" );
    if ( typ )
	fname.add( '_' ).add( typ );
    fname.add( '_' ).add( GetPID() ).add( '_' ).add( counter++ );
    if ( ext && *ext )
	{ fname.add( "." ).add( ext ); }
    return fname;
}


BufferString FilePath::getTempFullPath( const char* typ, const char* ext )
{
    return FilePath( getTempDir(), getTempFileName(typ,ext) ).fullPath();
}


BufferString FilePath::mkCleanPath( const char* path, Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    BufferString ret( path );
    if ( stl == Windows && !__iswin__ )
	ret = getCleanWinPath( path );
    if ( stl == Unix && __iswin__ )
	ret = getCleanUnxPath( path );

    return ret;
}


const char* FilePath::dirSep( Style stl )
{
    const char* wds = "\\";
    const char* uds = "/";

    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? wds : uds;
}


void FilePath::addPart( const char* fnm )
{
    if ( !fnm || !*fnm ) return;

    mSkipBlanks( fnm );
    const int maxlen = strlen( fnm );
    char prev = ' ';
    char* buf = new char [maxlen+1]; *buf = '\0';
    char* bufptr = buf;
    bool remdblsep = false;

    while ( *fnm )
    {
	char cur = *fnm;

	if ( cur != *dirSep(Local) && cur != *dirSep(cOther) )
	    remdblsep = true;
	else
	{
	    if ( (prev != *dirSep(Local) && prev != *dirSep(cOther))
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
    delete [] buf;
    trueDirIfLink();
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
	    lvls_.removeRange( idx-remoffs, idx );
	    idx -= remoffs + 1;
	}
    }
}


void FilePath::trueDirIfLink()
{
#ifdef __win__
    //TODO Launching OpendTect is taking too much time on windows platform.
    // Hence added a temporary fix. Need to find solution.
    static bool supportwinlinks = GetEnvVarYN( "OD_ALLOW_WINDOWS_LINKS" );
    if ( !supportwinlinks )
	return;

    BufferString dirnm = dirUpTo( -1 );
    if ( File::exists(dirnm) )
	return;

    dirnm += ".lnk";
    if ( File::exists(dirnm) && File::isLink(dirnm) )
    {
	const char* newdirnm = File::linkTarget( dirnm );
	set( newdirnm );
    }
#endif
}


BufferString FilePath::winDrive() const
{
    BufferString windrive = File::getRootPath( fullPath() );
    return windrive;
}


bool FilePath::isEmpty() const
{
    return prefix_.isEmpty() && lvls_.isEmpty();
}


BufferString FilePath::partitionName() const
{
    const QStorageInfo qsi( fullPath().buf() );
    const QString qstrdispnm( qsi.displayName() );
    return BufferString( qstrdispnm );
}


BufferString FilePath::rootPath() const
{
    const QStorageInfo qsi( fullPath().buf() );
    const QString qstrrootpath( qsi.rootPath() );
    return BufferString( qstrrootpath );
}
