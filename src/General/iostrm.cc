/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 25-10-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "iostrm.h"
#include "ioman.h"
#include "ascstream.h"
#include "strmprov.h"
#include "separstr.h"
#include "string2.h"
#include "file.h"
#include "filepath.h"
#include "perthreadrepos.h"
#include "oddirs.h"

class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return FixedString(typ)==StreamConn::sType(); }
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
		{ return new IOStream(nm,ky,fd); }

    static int	factoryid_;
};

int IOStreamProducer::factoryid_ = IOObj::addProducer( new IOStreamProducer );


IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
	: IOObj(nm,uid)
	, curfidx_(0)
{
    if ( mkdef )
	genFileName();
}


void IOStream::setDirName( const char* dirnm )
{
    IOObj::setDirName( dirnm );
    fs_.survsubdir_ = dirnm;
}


const char* IOStream::connType() const
{
    return StreamConn::sType();
}


bool IOStream::isBad() const
{
    return nrFiles() < 1;
}


void IOStream::copyFrom( const IOObj* obj )
{
    if ( !obj )
	return;

    IOObj::copyFrom( obj );
    mDynamicCastGet(const IOStream*,oth,obj)
    if ( oth )
    {
	fs_ = oth->fs_;
	curfidx_ = oth->curfidx_;
	extension_ = oth->extension_;
    }
}


const char* IOStream::fullUserExpr( bool forread ) const
{
    return fs_.absFileName( curfidx_ );
}


bool IOStream::implExists( bool fr ) const
{
    return File::exists( fullUserExpr(fr) );
}


bool IOStream::implReadOnly() const
{
    return !File::isWritable( fullUserExpr(true) );
}


bool IOStream::implRemove() const
{
    return implDoAll( true );
}


bool IOStream::implSetReadOnly( bool yn ) const
{
    return implDoAll( false, yn );
}


bool IOStream::implDoAll( bool dorem, bool yn ) const
{
    const int nrfiles = nrFiles();
    if ( nrfiles < 1 )
	return true;

    bool ret = true;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const BufferString fnm( fs_.absFileName(idx) );
	if ( dorem )
	    ret = File::remove( fnm ) && ret;
	else
	    ret = File::makeWritable( fnm, !yn, true ) && ret;
    }

    return ret;
}


bool IOStream::implRename( const char* newnm, const CallBack* cb )
{
    const int nrfiles = nrFiles();
    if ( nrfiles != 1 )
	return false;

    return File::rename( fullUserExpr(true), newnm );
}


int IOStream::connIdxFor( int nr ) const
{
    if ( !fs_.isRangeMulti() )
	return -1;
    return fs_.nrs_.nearestIndex( nr );
}


Conn* IOStream::getConn( bool forread ) const
{
    if ( isBad() )
	const_cast<IOStream*>(this)->genFileName();

    StreamConn*	ret = new StreamConn( fullUserExpr(forread), forread );
    if ( ret )
	ret->setLinkedTo( key() );

    return ret;
}


void IOStream::genFileName()
{
    BufferString fnm( name() );
    FilePath fp( fnm );
    const bool isabs = fp.isAbsolute();
    fnm.clean( isabs ? BufferString::NoSpaces : BufferString::AllowDots );
    const int extsz = extension_.size();
    const int neededsz = fnm.size() + extsz;
    if ( neededsz >= mMaxFilePathLength )
    {
	const BufferString uniqstr( "_",
			FilePath(FilePath::getTempName()).fileName() );
	const int len = uniqstr.size();
	fnm[ mMaxFilePathLength - len - extsz - 1 ] = '\0';
	fnm.add( uniqstr );
    }

    if ( extsz > 0 )
	fnm.add( "." ).add( extension_ );

    fs_.setFileName( fnm );
}


#define mStrmNext() { stream.next(); kw = stream.keyWord() + 1; }


bool IOStream::getFrom( ascistream& stream )
{
    fs_.setEmpty();

    BufferString kw = stream.keyWord() + 1;
    if ( kw == "Extension" )
	{ extension_ = stream.value(); mStrmNext() }
    if ( kw == "Multi" )
    {
	FileMultiString fms( stream.value() );
	StepInterval<int>& fnrs = fs_.nrs_;
	fnrs.start = fms.getIValue( 0 );
	fnrs.stop = fms.getIValue( 1 );
	fnrs.step = fms.getIValue( 2 );
	if ( fnrs.step == 0 ) fnrs.step = 1;
	if ( ( fnrs.start < fnrs.stop && fnrs.step < 0 )
	  || ( fnrs.stop < fnrs.start && fnrs.step > 0 ) )
	    Swap( fnrs.start, fnrs.stop );
	fs_.zeropad_ = fms.getIValue( 3 );
	curfidx_ = 0;
	mStrmNext()
    }

    BufferString fnm = stream.value();
    if ( kw == "Reader" )
    {
	fs_.fnames_.add( fnm );
	mStrmNext() // read "Writer"
	stream.next();
    }

    if ( !kw.startsWith("Name") )
	{ genFileName(); stream.next(); }
    else
    {
	fs_.fnames_.add( fnm );
	mStrmNext()
	while ( kw.startsWith("Name.") )
	{
	    fs_.fnames_.add( stream.value() );
	    mStrmNext()
	}
    }

    return true;
}


bool IOStream::putTo( ascostream& stream ) const
{
    int nrfiles = nrFiles();
    if ( nrfiles < 1 )
	{ stream.put( "$Name", "" ); return true; }

    if ( fs_.isRangeMulti() )
    {
	FileMultiString fms; const StepInterval<int>& fnrs = fs_.nrs_;
	fms += fnrs.start; fms += fnrs.stop; fms += fnrs.step;
	fms += fs_.zeropad_;
	stream.put( "$Multi", fms );
    }

    const FilePath fpsurvsubdir( GetDataDir(), dirName() );
    const BufferString survsubdir( fpsurvsubdir.fullPath() );
    nrfiles = fs_.fnames_.size();

    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FilePath fp( fs_.fnames_.get(idx) );
	BufferString fnm( fp.fullPath() );
	int offs = 0;
	if ( fp.isAbsolute() )
	{
	    BufferString head( fp.dirUpTo( fpsurvsubdir.nrLevels() - 1 ) );
	    if ( head == survsubdir )
		offs = head.size()+1;
	}
	if ( idx == 0 )
	    stream.put( "$Name", fnm.buf() + offs );
	else
	    stream.put( BufferString("$Name.",idx).buf(), fnm.buf() + offs );
    }

    return true;
}

