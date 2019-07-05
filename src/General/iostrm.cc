/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 25-10-1994
-*/


#include "iostrm.h"
#include "ascstream.h"
#include "separstr.h"
#include "string2.h"
#include "file.h"
#include "filepath.h"
#include "transl.h"
#include "staticstring.h"
#include "surveydisklocation.h"
#include "envvars.h"
#include "oddirs.h"


static void getFullSpecFileName( BufferString& fnm, BufferString* specfnm )
{
    if ( !fnm.startsWith( "${" ) )
	return;

    BufferString dirnm( fnm.str()+2 );
    char* ptr = dirnm.find( "}" );
    if ( !ptr )
	{ fnm = dirnm; return; }

    if ( specfnm )
	*specfnm = fnm;
    *ptr++ = '\0';
    fnm = ptr;

    if ( dirnm == "DTECT_DATA" )
	dirnm = GetBaseDataDir();
    else if ( dirnm == "DTECT_APPL" )
	dirnm = GetSoftwareDir(true);
    else if ( dirnm == "DTECT_APPL_SETUP" )
	dirnm = GetApplSetupDir();
    else if ( dirnm == "DTECT_SETTINGS" )
	dirnm = GetSettingsDir();
    else
    {
	dirnm = GetEnvVar( dirnm );
	if ( dirnm.isEmpty() )
	    dirnm = GetBaseDataDir();
    }

    File::Path fp( dirnm, fnm );
    fnm = fp.fullPath();
}


class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return FixedString(typ)==StreamConn::sType(); }
    IOObj*	make( const char* nm, const DBKey& ky, bool fd ) const
		{ return new IOStream(nm,ky,fd); }

    static int	factoryid_;
};

int IOStreamProducer::factoryid_ = IOObj::addProducer( new IOStreamProducer );


IOStream::IOStream( const char* nm, DBKey ky, bool mkdef )
	: IOObj(nm,ky)
	, curfidx_(0)
{
    if ( mkdef )
	genFileName();
}


IOStream::IOStream( const IOStream& oth )
{
    copyFrom( oth );
}


void IOStream::setDirName( const char* dirnm )
{
    IOObj::setDirName( dirnm );
    fs_.survsubdir_ = dirnm;
}


void IOStream::setAbsDirectory( const char* dirnm )
{
    File::Path fp( dirnm );
    setDirName( fp.fileName() );
    fs_.makeAbsoluteIfRelative( dirnm );
}


bool IOStream::isInCurrentSurvey() const
{
    return fs_.survsubdir_.isEmpty() ? true : key().isInCurrentSurvey();
}


DBKey IOStream::key() const
{
    DBKey dbky( key_ );
    if ( fs_.survsubdir_.isEmpty() )
	return dbky;

    File::Path fp( fs_.absFileName() );
    while ( true )
    {
	const BufferString dirnm( fp.fileName() );
	if ( dirnm.isEmpty() )
	    break;

	fp.setFileName( 0 );
	if ( dirnm == fs_.survsubdir_ )
	{
	    const BufferString survdirnm( fp.fileName() );
	    fp.setFileName( 0 );
	    const SurveyDiskLocation sdl( survdirnm, fp.fullPath() );
	    if ( !sdl.isCurrentSurvey() )
		dbky.setSurveyDiskLocation( sdl );
	    break;
	}
    }

    return dbky;
}


const char* IOStream::connType() const
{
    return StreamConn::sType();
}


bool IOStream::isBad() const
{
    return nrFiles() < 1;
}


bool IOStream::isEqTo( const IOObj& othioobj ) const
{
    mDynamicCastGet( const IOStream*, oth, &othioobj )
    if ( !oth )
	return false;

    return fs_ == oth->fs_
	&& extension_ == oth->extension_
	&& specfname_ == oth->specfname_;
}


void IOStream::copyFrom( const IOObj& obj )
{
    IOObj::copyFrom( obj );
    mDynamicCastGet(const IOStream*,oth,&obj)
    if ( oth )
    {
	fs_ = oth->fs_;
	curfidx_ = oth->curfidx_;
	extension_ = oth->extension_;
	specfname_ = oth->specfname_;
    }
}


const char* IOStream::fullUserExpr( bool forread ) const
{
    mDeclStaticString( ret );
    ret = mainFileName();
    return ret;
}


BufferString IOStream::mainFileName() const
{
    BufferString ret( fs_.absFileName(curfidx_) );
    getFullSpecFileName( ret, 0 );
    return ret;
}


bool IOStream::implExists( bool fr ) const
{
    return File::exists( mainFileName() );
}


bool IOStream::implReadOnly() const
{
    return File::exists( mainFileName() ) && !File::isWritable( mainFileName());
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
	{
	    if ( File::exists(fnm) )
		ret = File::remove( fnm ) && ret;
	}
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

    const BufferString newfnm( newnm );
    const BufferString oldfnm( mainFileName() );
    if ( !File::rename(oldfnm,newfnm) )
	return false;
    if ( specfname_.isEmpty() )
	return true;

    File::Path fpnew( newfnm );
    File::Path newfpspec( specfname_ );
    newfpspec.setFileName( fpnew.fileName() );
    BufferString newfullspecfnm = newfpspec.fullPath();
    getFullSpecFileName( newfullspecfnm, 0 );
    if ( newfnm == newfullspecfnm )
	specfname_ = newfpspec.fullPath();

    return true;
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

    BufferString fnm( mainFileName() );
    if ( fnm.isEmpty() )
	return 0;

    PtrMan<Translator> trl = createTranslator();
    if ( trl )
	trl->convToConnExpr( fnm );

    StreamConn*	ret = new StreamConn( fnm, forread );
    if ( ret )
	ret->setLinkedTo( key() );

    return ret;
}


void IOStream::genFileName()
{
    BufferString fnm( name() );
    File::Path fp( fnm );
    const bool isabs = fp.isAbsolute();
    fnm.clean( isabs ? BufferString::NoSpaces : BufferString::AllowDots );
    const int extsz = extension_.size();
    const int neededsz = fnm.size() + extsz;
    if ( neededsz >= mMaxFilePathLength )
    {
	const BufferString uniqstr( "_", File::Path::getTimeStampFileName(0) );
	const int len = uniqstr.size();
	fnm[ mMaxFilePathLength - len - extsz - 1 ] = '\0';
	fnm.add( uniqstr );
    }

    if ( extsz > 0 )
	fnm.add( "." ).add( extension_ );

    fs_.setFileName( fnm );
}


bool IOStream::locateInSharedDir( const char* dirnm )
{
    File::Path shfp( GetBaseDataDir(), dirnm );
    const BufferString shdirnm( shfp.fullPath() );
    if ( !File::isDirectory(shdirnm) )
    {
	if ( File::exists(shdirnm) )
	    return false;
	else if ( !File::isWritable(GetBaseDataDir()) )
	    return false;
	else if ( !File::createDir(shdirnm) )
	    return false;
    }

    File::Path curfp( mainFileName() );
    const File::Path newfp( "${DTECT_DATA}", dirnm, curfp.fileName() );
    fs_.setFileName( newfp.fullPath() );
    return true;
}


#define mStrmNext() { stream.next(); kw = stream.keyWord() + 1; }


bool IOStream::getFrom( ascistream& stream )
{
    fs_.setEmpty();
    specfname_.setEmpty();

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
	    std::swap( fnrs.start, fnrs.stop );
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
	getFullSpecFileName( fnm, &specfname_ );
	fs_.fnames_.add( File::Path(fnm).fullPath() );
	mStrmNext()
	while ( kw.startsWith("Name.") )
	{
	    fnm = stream.value();
	    getFullSpecFileName( fnm, 0 );
	    fs_.fnames_.add( File::Path(stream.value()).fullPath() );
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

    const File::Path fpsurvsubdir( GetDataDir(), dirName() );
    const BufferString survsubdir( fpsurvsubdir.fullPath() );
    nrfiles = fs_.fnames_.size();

    for ( int idx=0; idx<nrfiles; idx++ )
    {
	File::Path fp( fs_.fnames_.get(idx) );
	BufferString fnm( fp.fullPath() );
	bool trymakerel = fp.isAbsolute();
	if ( idx == 0 && !specfname_.isEmpty() )
	{
	    BufferString fullspecfnm = specfname_;
	    getFullSpecFileName( fullspecfnm, 0 );
	    if ( fullspecfnm == fnm )
	    {
		fnm = specfname_;
		trymakerel = false;
	    }
	}
	BufferString omffnm( fnm );
	if ( trymakerel )
	{
	    BufferString head( fp.dirUpTo( fpsurvsubdir.nrLevels() - 1 ) );
	    if ( head == survsubdir )
		omffnm = fnm.buf() + head.size()+1;
	    else
	    {
		head.set( fp.dirUpTo( fpsurvsubdir.nrLevels() - 2 ) );
		if ( head == GetDataDir() )
		{
		    File::Path fpomf( "..", fnm.buf() + head.size()+1 );
		    omffnm.set( fpomf.fullPath() );
		}
	    }
	}
	if ( idx == 0 )
	    stream.put( "$Name", omffnm );
	else
	    stream.put( BufferString("$Name.",idx).buf(), omffnm );
    }

    return true;
}
