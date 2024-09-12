/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filepath.h"

#include "envvars.h"
#include "file.h"
#include "stringview.h"
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


FilePath::~FilePath()
{}


FilePath& FilePath::operator =( const FilePath& oth )
{
    lvls_ = oth.lvls_;
    prefix_ = oth.prefix_;
    postfix_ = oth.postfix_;
    domain_ = oth.domain_;
    isabs_ = oth.isabs_;
    isuri_ = oth.isuri_;
    return *this;
}


FilePath& FilePath::operator =( const char* fnm )
{ return (*this = FilePath(fnm)); }


bool FilePath::operator ==( const FilePath& oth ) const
{
    return lvls_ == oth.lvls_ &&
	   prefix_ == oth.prefix_ && postfix_ == oth.postfix_ &&
	   domain_ == oth.domain_ && isabs_ == oth.isabs_ &&
	   isuri_ == oth.isuri_;
}


bool FilePath::operator ==( const char* fnm ) const
{ return *this == FilePath(fnm); }


bool FilePath::operator !=( const FilePath& oth ) const
{ return !(*this == oth); }


bool FilePath::operator != ( const char* fnm ) const
{ return !(*this == FilePath(fnm)); }


static bool isServerPath( const char* path )
{
    const StringView pathstr = path;
    return pathstr.size()>1 && path[0]=='\\' && path[1]=='\\';
}


BufferString FilePath::getFullLongPath( const FilePath& fp )
{
    return getLongPath( fp.fullPath().buf(), nullptr );
}


BufferString FilePath::getLongPath( const char* shortpath,
				    BufferString* error )
{
    BufferString res;
#ifndef  __win__
    res = shortpath;
#else
    res.setMinBufSize( MAX_PATH );
    const int len = GetLongPathName( shortpath, res.getCStr(), MAX_PATH );
    if ( len==0 || len>MAX_PATH )
    {
	DWORD errorcode = GetLastError();
	if ( error )
	    error->set("Could not convert path: ").add( sCast(int,errorcode) );
//	Implement FormatMessage for a better error message
	res = shortpath;
    }
#endif
    return res;
}


BufferString FilePath::getShortPath( const char* longpath,
				     BufferString* error )
{
    BufferString res;
#ifndef  __win__
    res = longpath;
#else
    res.setMinBufSize( MAX_PATH );
    const int len = GetShortPathName( longpath, res.getCStr(), MAX_PATH );
    if ( len==0 || len>MAX_PATH )
    {
	DWORD errorcode = GetLastError();
	if ( error )
	    error->set("Could not convert path: ").add( sCast(int,errorcode) );
//	Implement FormatMessage for a better error message
	res = longpath;
    }
#endif
    return res;
}


FilePath& FilePath::set( const char* inpfnm )
{
    lvls_.erase();
    prefix_.setEmpty();
    postfix_.setEmpty();
    domain_.setEmpty();
    isabs_ = false;
    isuri_ = false;
    if ( !inpfnm || !*inpfnm )
	return *this;

    const BufferString fnmbs( inpfnm );
    const char* fnm = fnmbs.buf();
    mSkipBlanks( fnm );
    if ( !*fnm )
	return *this;

    if ( File::isURI(fnm) )
    {
	isuri_ = true;
	BufferString fnmcopy( fnm );
	char* sepptr = fnmcopy.find( uriProtocolSeparator() );
	*sepptr = '\0';
	prefix_ = fnmcopy;
	char* domptr = sepptr + 3;
	while ( *domptr && *domptr != '/' )
	{
	    domain_.add( *domptr );
	    domptr++;
	}

	fnm = nullptr;
	if ( *domptr == '/' )
	    fnm = fnmbs.buf() + (domptr - fnmcopy.buf());
	isabs_ = true;
    }
    else
    {
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
    }

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

    const FilePath fp( fnm );
    for ( int idx=fp.nrLevels()-1; idx>=0; idx-- )
	lvls_.insertAt( new BufferString(fp.dir(idx)), 0 );

    if ( fp.isAbsolute() )
	prefix_ = fp.prefix();

    if ( fp.isURI() )
	domain_ = fp.domain_;

    isabs_ = isAbsolute() || fp.isAbsolute();
    isuri_ = isURI() || fp.isURI();
    compress();
    return *this;
}


FilePath& FilePath::setFileName( const char* fnm )
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

    return *this;
}


FilePath& FilePath::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ?
	    lvls_.get(lvls_.size()-1).buf() : (const char*) 0 );
    set( pth );
    if ( !fnm.isEmpty() )
	add( fnm );

    return *this;
}


FilePath& FilePath::setExtension( const char* ext, bool replace )
{
    if ( !ext ) ext = "";
    mSkipBlanks( ext );

    if ( *ext == '.' )
	ext++;
    if ( lvls_.size() < 1 )
    {
	if ( *ext )
	    add( ext );
	return *this;
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
	fname.add( "." ).add( ext );

    return *this;
}


bool FilePath::exists() const
{
    return File::exists( fullPath() );
}


bool FilePath::isAbsolute() const
{
    return isabs_;
}


bool FilePath::isURI() const
{
    return isuri_;
}


bool FilePath::isSubDirOf( const FilePath& oth, FilePath* relpath ) const
{
    if ( oth.isabs_ != isabs_ || oth.prefix_ != prefix_ ||
	 oth.domain_ != domain_ )
	return false;

    const int nrlvls = nrLevels();
    const int othnrlvls = oth.nrLevels();
    if ( othnrlvls >= nrlvls )
	return false;

    for ( int idx=0; idx<othnrlvls; idx++ )
    {
	if ( *lvls_[idx] != *oth.lvls_[idx] )
	    return false;
    }

    if ( relpath )
    {
	BufferString rel;
	for ( int idx=othnrlvls; idx<nrlvls; idx++ )
	{
	    if ( idx > othnrlvls )
		rel.add( dirSep() );
	    rel.add( dir(idx) );
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


bool FilePath::makeRelativeTo( const FilePath& oth )
{
    const BufferString file = fullPath();
    const BufferString path = oth.fullPath();
    set( File::getRelativePath( path.buf(), file.buf() ) );
    return true;
}


bool FilePath::makeAbsolute( const FilePath& oth )
{
    if ( isAbsolute() )
	return true;

    if ( !oth.isAbsolute() )
	return false;

    const BufferString relpath = fullPath();
    const BufferString path = oth.fullPath();
    set( File::getAbsolutePath(path.buf(),relpath.buf()) );
    return true;
}


BufferString FilePath::fullPath( Style f, bool cleanup ) const
{
    BufferString res = dirUpTo( lvls_.size() );
    if ( cleanup )
	res = mkCleanPath( res, f );

    if ( isabs_ && ((__iswin__ && f==Local) || f==Windows) && nrLevels() < 1 )
	res.add( dirSep(Windows) );

    if ( !postfix_.isEmpty() )
    {
	if ( isURI() )
	    res.add( "?" );

	res.add( postfix_ );
    }

    return res;
}


const char* FilePath::prefix() const
{
    return prefix_.buf();
}

const char* FilePath::postfix() const
{
    return postfix_.buf();
}

const char* FilePath::domain() const
{
    return domain_.buf();
}

void FilePath::setDomain( const char* dom )
{
    domain_ = dom;
}

int FilePath::nrLevels() const
{
    return lvls_.size();
}


const char* FilePath::extension() const
{
    if ( lvls_.isEmpty() )
	return 0;

    const char* ret = lastOcc( fileName().buf(), '.' );
    if ( ret )
	ret++;
    return ret;
}


const OD::String& FilePath::fileName() const
{
    return dir(-1);
}


BufferString FilePath::baseName() const
{
    FilePath selfcopy( *this );
    selfcopy.setExtension( nullptr );
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


BufferString FilePath::pathOnly( Style f, bool cleanup ) const
{
    BufferString res = dirUpTo( lvls_.size()-2 );
    if ( cleanup )
	res = mkCleanPath( res, f );

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
    const int nrlvls = lvls_.size();
    if ( lvl > nrlvls-1 )
	lvl = nrlvls - 1;

    const bool isuri = isURI();
    BufferString ret;
    if ( !prefix_.isEmpty() )
    {
	ret.set( prefix_ );
	if ( isuri )
	    ret.add( uriProtocolSeparator() ).add( domain_ );
	else
	    ret.add( sPrefSep );
    }
    if ( lvl < 0 )
	return ret;

    if ( isabs_ )
	ret.add( dirSep() );
    if ( nrlvls > 0 )
	ret.add( lvls_.get( 0 ) );

    for ( int idx=1; idx<=lvl; idx++ )
	ret.add( dirSep() ).add( lvls_.get(idx) );

    return ret;
}


BufferString FilePath::fileFrom( int lvl, Style st ) const
{
    BufferString ret;
    if ( isURI() )
	st = Unix;

    const int sz = lvls_.size();
    for ( int ilvl=lvl; ilvl<sz; ilvl++ )
    {
	ret.add( lvls_.get( ilvl ) );
	if ( ilvl != sz-1 )
	    ret.add( dirSep(st) );
    }

    return ret;
}


BufferString FilePath::getTempDir()
{
    BufferString tmpdir = File::getTempPath();
    if ( !File::exists(tmpdir) )
    {
	BufferString msg( "Temporary folder '", tmpdir, "'does not exist" );
	UsrMsg( msg );
    }
    else if ( !File::isWritable(tmpdir) )
    {
	BufferString msg( "Temporary folder '", tmpdir, "'is read-only" );
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
	ret = getCleanUnixPath( path );

    return ret;
}


static const char* sWindowDirSep = "\\";
static const char* sUnixDirSep = "/";

const char* FilePath::dirSep() const
{
    return isURI() ? sUnixDirSep : dirSep( Local );

}

const char* FilePath::dirSep( Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? sWindowDirSep : sUnixDirSep;
}


void FilePath::addPart( const char* fnm )
{
    if ( !fnm || !*fnm )
	return;

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
    if ( buf[0] )
    {
	if ( isuri_ && lvls_.isEmpty() && domain_.isEmpty() )
	    domain_ = buf;
	else
	    lvls_.add( buf );
    }

    delete [] buf;
    if ( lvls_.isEmpty() )
	return;

    if ( isURI() )
    {
	BufferString& lastlvl = *lvls_.last();
	char* postfixptr = lastlvl.find( '?' );
	if ( postfixptr )
	{
	    *postfixptr++ = '\0';
	    if ( *postfixptr )
		postfix_ = postfixptr;
	}
    }
}


void FilePath::compress( int startlvl )
{
    for ( int idx=startlvl; idx<lvls_.size(); idx++ )
    {
	const BufferString& bs = lvls_.get( idx );
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
	const char* newdirnm = File::linkEnd( dirnm );
	set( newdirnm );
    }
#endif
}
