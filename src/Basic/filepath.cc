/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2004
-*/


#include "filepath.h"

#include "envvars.h"
#include "file.h"
#include "fixedstring.h"
#include "genc.h"
#include "msgh.h"
#include "oddirs.h"
#include "staticstring.h"
#include "timefun.h"

#include <string.h>


const char* File::Path::sPrefSep = ":";

static const File::Path::Style cOtherStyle = __iswin__ ? File::Path::Unix
						       : File::Path::Windows;

File::Path::Path( const char* fnm )
{
    set( fnm );
}


File::Path::Path( const char* p1, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    set( p1 );
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


File::Path::Path( const Path& oth, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    *this = oth;
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


File::Path& File::Path::operator =( const Path& oth )
{
    lvls_ = oth.lvls_;
    prefix_ = oth.prefix_;
    domain_ = oth.domain_;
    postfix_ = oth.postfix_;
    isabs_ = oth.isabs_;
    return *this;
}


File::Path& File::Path::operator =( const char* fnm )
{
    return (*this = Path(fnm));
}


bool File::Path::operator ==( const Path& oth ) const
{
    return lvls_ == oth.lvls_ && domain_ == oth.domain_
	&& prefix_ == oth.prefix_ && postfix_ == oth.postfix_;
}


bool File::Path::operator ==( const char* fnm ) const
{
    return *this == Path(fnm);
}


bool File::Path::operator !=( const Path& oth ) const
{
    return !(*this == oth);
}


bool File::Path::operator !=( const char* fnm ) const
{
    return !(*this == Path(fnm));
}


static bool isServerPath( const char* path )
{
    const FixedString pathstr = path;
    return pathstr.size()>1 && path[0]=='\\' && path[1]=='\\';
}


BufferString File::Path::getFullLongPath( const File::Path& fp )
{
    if ( !__iswin__ )
	return fp.fullPath();

    mDeclStaticString( longpath );
    longpath.setMinBufSize( 1025 );
#ifdef __win__
    GetLongPathName(fp.fullPath(), longpath.getCStr(), longpath.minBufSize()-1);
#endif
    BufferString fpstr = longpath;
    if ( fpstr.isEmpty() )
	fpstr = fp.fullPath();
    return fpstr;
}


File::Path& File::Path::set( const char* inpfnm )
{
    lvls_.erase();
    prefix_.setEmpty(); postfix_.setEmpty(); domain_.setEmpty();
    isabs_ = false;
    if ( !inpfnm || !*inpfnm )
	return *this;

    const BufferString fnmbs( inpfnm );
    const char* fnm = fnmbs.buf();
    mSkipBlanks( fnm );
    if ( !*fnm )
	return *this;

    if ( File::isURI(fnm) )
    {
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
	    const char* otherdsptr = firstOcc( fnm, *dirSep(cOtherStyle) );
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
	if ( isabs_ && *fnm)
	    fnm++;
    }

    addPart( fnm );
    compress();
    return *this;
}


File::Path& File::Path::add( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    int sl = lvls_.size();
    addPart( fnm );
    compress( sl );

    return *this;
}


File::Path& File::Path::insert( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    BufferStringSet oldlvls( lvls_ );
    lvls_.setEmpty();
    set( fnm );
    lvls_.append( oldlvls );
    return *this;
}


File::Path& File::Path::setFileName( const char* fnm )
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


File::Path& File::Path::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ?
	    lvls_.get(lvls_.size()-1).buf() : (const char*) 0 );
    set( pth );
    if ( !fnm.isEmpty() )
	add( fnm );
    return *this;
}


File::Path& File::Path::setExtension( const char* ext, bool replace )
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


bool File::Path::exists() const
{
    return File::exists( fullPath() );
}


bool File::Path::isSubDirOf( const Path& oth, Path* relpath ) const
{
    if ( oth.isabs_ != isabs_ || oth.prefix_ != prefix_
	|| oth.domain_ != domain_ )
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
	    rel.add( dir( idx ) );
	}

	relpath->set( rel.buf() );
    }

    return true;
}


bool File::Path::makeCanonical()
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


bool File::Path::makeRelativeTo( const Path& oth )
{
    const BufferString file = fullPath();
    const BufferString path = oth.fullPath();
    set( File::getRelativePath( path.buf(), file.buf() ) );
    return true;
}



BufferString File::Path::fullPath( Style f, bool cleanup ) const
{
    BufferString res = dirUpTo(-1);
    if ( cleanup )
	res = mkCleanPath( res, f );
    if ( isabs_ && ((__iswin__ && f==Local) || f==Windows) && nrLevels() < 1 )
	res.add( dirSep(Windows) );
    if ( postfix_ )
	res.add( postfix_ );
    return res;
}


const char* File::Path::extension() const
{
    if ( lvls_.isEmpty() )
	return 0;

    const char* ret = lastOcc( fileName().buf(), '.' );
    if ( ret )
	ret++;
    return ret;
}


const OD::String& File::Path::fileName() const
{
    return dir( -1 );
}


BufferString File::Path::baseName() const
{
    File::Path selfcopy( *this );
    selfcopy.setExtension( 0 );
    return selfcopy.fileName();
}


BufferString File::Path::getTimeStampFileName( const char* ext )
{
    BufferString tsfnm;
    BufferString datestr = Time::getISOUTCDateTimeString();
    datestr.replace( ", ", "-" );
    datestr.replace( ':', '.' );
    datestr.replace( ' ', '_' );
    tsfnm.add( datestr );
    if ( ext && *ext )
    {
	if ( *ext == '.' )
	    ext++;
	if ( *ext )
	    tsfnm.add( '.' ).add( ext );
    }

    return tsfnm;
}


BufferString FilePath::pathOnly() const
{
    return pathOnly( Local );
}


BufferString FilePath::pathOnly( Style f ) const
{
    BufferString res = dirUpTo( lvls_.size()-2 );
    if ( isabs_ && ((__iswin__ && f==Local) || f==Windows) && nrLevels() < 2 )
	res.add( dirSep(Windows) );

    return res;
}


const OD::String& File::Path::dir( int nr ) const
{
    if ( nr < 0 || nr >= lvls_.size() )
	nr = lvls_.size()-1;
    return nr < 0 ? BufferString::empty() : *lvls_[nr];
}


BufferString File::Path::dirUpTo( int lvl ) const
{
    const int nrlvls = lvls_.size();
    if ( lvl < 0 || lvl > nrlvls-1 )
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
	ret.add( dirSep() ).add( lvls_.get( idx ) );

    return ret;
}


BufferString File::Path::fileFrom( int lvl ) const
{
    BufferString ret;

    const int sz = lvls_.size();
    for ( int ilvl=lvl; ilvl<sz; ilvl++ )
    {
	ret.add( lvls_.get( ilvl ) );
	if ( ilvl != sz-1 )
	    ret.add( dirSep() );
    }

    return ret;
}


BufferString File::Path::getTempDir()
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


BufferString File::Path::getTempFileName( const char* typ, const char* ext )
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


BufferString File::Path::getTempFullPath( const char* typ, const char* ext )
{
    return Path( getTempDir(), getTempFileName(typ,ext) ).fullPath();
}


BufferString File::Path::mkCleanPath( const char* path, Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    BufferString ret( path );
    if ( stl == Windows && !__iswin__ )
	ret = GetCleanWinPath( path );
    if ( stl == Unix && __iswin__ )
	ret = GetCleanUnxPath( path );

    return ret;
}

static const char* winds = "\\";
static const char* unixds = "/";

const char* File::Path::dirSep() const
{
    return isURI() ? unixds : dirSep( Local );
}


const char* File::Path::dirSep( Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? winds : unixds;
}


void File::Path::addPart( const char* fnm )
{
    if ( !fnm || !*fnm ) return;

    mSkipBlanks( fnm );
    const int maxlen = strLength( fnm );
    char prev = ' ';
    char* buf = new char [maxlen+1]; *buf = '\0';
    char* bufptr = buf;
    bool remdblsep = false;

    while ( *fnm )
    {
	char cur = *fnm;

	if ( cur != *dirSep(Local) && cur != *dirSep(cOtherStyle) )
	    remdblsep = true;
	else
	{
	    if ( (prev != *dirSep(Local) && prev != *dirSep(cOtherStyle))
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

    conv2TrueDirIfLink();
}


void File::Path::compress( int startlvl )
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


void File::Path::conv2TrueDirIfLink()
{
#ifdef __win__
    //TODO Launching OpendTect is taking too much time on windows platform.
    // Hence added a temporary fix. Need to find solution.
    static bool supportwinlinks = GetEnvVarYN( "OD_ALLOW_WINDOWS_LINKS" );
    if ( !supportwinlinks || isURI() )
	return;

    BufferString dirnm = dirUpTo( -1 );
    if ( File::exists(dirnm) )
	return;

    dirnm += ".lnk";
    if ( File::exists(dirnm) && File::isLink(dirnm) )
	set( File::linkEnd(dirnm) );
#endif
}


BufferString File::Path::winDrive() const
{
    BufferString windrive = File::getRootPath( fullPath() );
    return windrive;
}
