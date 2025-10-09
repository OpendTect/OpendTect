/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "iostrm.h"

#include "ascstream.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oduuid.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "string2.h"
#include "strmprov.h"
#include "survinfo.h"


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
	dirnm = GetApplSetupDataDir();
    else if ( dirnm == "DTECT_SETTINGS" )
	dirnm = GetSettingsDir();
    else
    {
	dirnm = GetEnvVar( dirnm );
	if ( dirnm.isEmpty() )
	    dirnm = GetBaseDataDir();
    }

    FilePath fp( dirnm, fnm );
    fnm = fp.fullPath();
}


class IOStreamProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const override
		{ return StringView(typ)==StreamConn::sType(); }
    IOObj*	make( const char* nm,
		      const DBKey& ky, bool fd ) const override
		{ return new IOStream(nm,ky,fd); }

    static int	factoryid_;
};

int IOStreamProducer::factoryid_ = IOObj::addProducer( new IOStreamProducer );


// IOStream

IOStream::IOStream( const char* nm, const DBKey& ky, bool mkdef )
    : IOObj(nm,ky)
    , fs_(ky)
{
    if ( mkdef )
	genFileName();
}


IOStream::IOStream()
    : IOStream(nullptr,DBKey(MultiID::udf(),SI().diskLocation()))
{
}


IOStream::IOStream( const char* nm, const MultiID& ky, bool mkdef )
    : IOStream(nm,DBKey(ky,SI().diskLocation()),mkdef)
{
}

mStartAllowDeprecatedSection

IOStream::IOStream( const char* nm, const char* uid, bool mkdef )
    : IOStream(nm,DBKey(MultiID(uid),SI().diskLocation()),mkdef)
{
}

mStopAllowDeprecatedSection


IOStream::IOStream( const IOStream& oth )
    : IOObj(nullptr,DBKey(MultiID::udf()))
{
    copyFrom( &oth );
}


IOStream::~IOStream()
{}


void IOStream::setDirName( const char* dirnm )
{
    IOObj::setDirName( dirnm );
    fs_.survsubdir_ = dirnm;
}


void IOStream::setAbsDirectory( const char* dirnm )
{
    FilePath fp( dirnm );
    setDirName( fp.fileName() );
    fs_.makeAbsoluteIfRelative( dirnm );
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
    return File::exists( mainFileName() ) &&
	(!File::isWritable( mainFileName()) || !isUserSelectable(false) );
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
	    ret = File::setWritable( fnm, !yn, true ) && ret;
    }

    return ret;
}


bool IOStream::implRename( const char* newnm )
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

    FilePath fpnew( newfnm );
    FilePath newfpspec( specfname_ );
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
	getNonConst(*this).genFileName();
    else if ( !forread && transl_ == "Blocks" )
	return nullptr; // protect 6.X users against removing their data

    auto* ret = new StreamConn( mainFileName(), forread );
    if ( ret )
	ret->setLinkedTo( key() );

    return ret;
}


void IOStream::genFileName()
{
    if ( GetEnvVarYN("OD_UUID_FILENAME",false) )
    {
	const BufferString fnm = OD::Uuid::create( false );
	if ( !fnm.isEmpty() )
	{
	    FilePath fp( fnm );
	    fp.setExtension( extension_.buf() );
	    fs_.setFileName( fp.fullPath() );
	    return;
	}
    }

    BufferString fnm( name() );
    FilePath fp( fnm );
    const bool isabs = fp.isAbsolute();
    fnm.clean( isabs ? BufferString::NoSpaces : BufferString::AllowDots );
    const int extsz = extension_.size();
    const int neededsz = fnm.size() + extsz;
    if ( neededsz >= mMaxFilePathLength )
    {
	const BufferString uniqstr( "_",
				   FilePath::getTimeStampFileName(nullptr) );
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
    specfname_.setEmpty();

    BufferString kw = stream.keyWord() + 1;
    if ( kw == "Extension" )
	{ extension_ = stream.value(); mStrmNext() }
    if ( kw == "Multi" )
    {
	FileMultiString fms( stream.value() );
	StepInterval<int>& fnrs = fs_.nrs_;
	fnrs.start_ = fms.getIValue( 0 );
	fnrs.stop_ = fms.getIValue( 1 );
	fnrs.step_ = fms.getIValue( 2 );
	if ( fnrs.step_ == 0 ) fnrs.step_ = 1;
	if ( ( fnrs.start_ < fnrs.stop_ && fnrs.step_ < 0 )
	     || ( fnrs.stop_ < fnrs.start_ && fnrs.step_ > 0 ) )
	    Swap( fnrs.start_, fnrs.stop_ );
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
	fs_.fnames_.add( FilePath(fnm).fullPath() );
	mStrmNext()
	while ( kw.startsWith("Name.") )
	{
	    fnm = stream.value();
	    getFullSpecFileName( fnm, 0 );
	    fs_.fnames_.add( FilePath(stream.value()).fullPath() );
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
	FileMultiString fms;
	const StepInterval<int>& fnrs = fs_.nrs_;
	fms += fnrs.start_; fms += fnrs.stop_; fms += fnrs.step_;
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
		    FilePath fpomf( "..", fnm.buf() + head.size()+1 );
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
