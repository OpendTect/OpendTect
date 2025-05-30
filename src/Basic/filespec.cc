/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filespec.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "surveydisklocation.h"

const char* FileSpec::sKeyFileNrs()	   { return "File numbers"; }


FileSpec::FileSpec( const char* fnm )
    : nrs_(mUdf(int),0,1)
{
    if ( fnm && *fnm )
	fnames_.add( fnm );
}


FileSpec::FileSpec( const DBKey& ky )
    : nrs_(mUdf(int),0,1)
{
    if ( ky.hasSurveyLocation() && ky.groupID() >= 0 )
    {
	const SurveyDiskLocation& sdl = ky.surveyDiskLocation();
	const BufferString datadir = sdl.fullPath( false );
	if ( File::exists(datadir) )
	    datadir_ = datadir;
    }
}


FileSpec::FileSpec( const IOPar& iop )
    : nrs_(mUdf(int),0,1)
{
    usePar( iop );
}


FileSpec::~FileSpec()
{}


int FileSpec::nrFiles() const
{
    const int nrfnms = fnames_.size();
    if ( nrfnms > 1 )
	return nrfnms;

    return mIsUdf(nrs_.start_) ? nrfnms : nrs_.nrSteps() + 1;
}


bool FileSpec::isRangeMulti() const
{
    const int nrfnms = fnames_.size();
    return nrfnms >= 1 && !mIsUdf(nrs_.start_);
}


const char* FileSpec::dirName() const
{
    FilePath fp( fullDirName() );
    mDeclStaticString( ret );
    ret = fp.fileName();
    return ret.buf();
}


const char* FileSpec::fullDirName() const
{
    FilePath fp( fileName(0) );
    if ( fp.isAbsolute() )
	fp.setFileName( nullptr );
    else
    {
	if ( datadir_.isEmpty() )
	    { pErrMsg("Should not be reached"); }

	fp.set( datadir_ );
	if ( !survsubdir_.isEmpty() )
	    fp.add( survsubdir_ );
    }

    mDeclStaticString( ret );
    ret = fp.fullPath();
    return ret.buf();
}


const char* FileSpec::usrStr() const
{
    return usrstr_.isEmpty() ? fileName( 0 ) : usrstr_.buf();
}


const char* FileSpec::fileName( int fidx ) const
{
    const int nrfnms = fnames_.size();
    if ( fidx < 0 || nrfnms < 1 )
	return "";

    if ( nrfnms > 1 )
	return fidx < nrfnms ? fnames_.get( fidx ).buf() : "";

    const int nrfiles = nrFiles();
    if ( fidx >= nrfiles )
	return "";
    else if ( mIsUdf(nrs_.start_) )
	return nrfnms < 1 ? "" : fnames_.get(fidx).buf();

    const int nr = nrs_.atIndex( fidx );
    BufferString replstr;
    if ( zeropad_ < 2 )
	replstr.set( nr );
    else
    {
	BufferString numbstr; numbstr.set( nr );
	const int numblen = numbstr.size();
	while ( numblen + replstr.size() < zeropad_ )
	    replstr.add( "0" );
	replstr.add( numbstr );
    }

    mDeclStaticString(ret); ret = fnames_.get( 0 );
    ret.replace( "*", replstr.buf() );
    return ret.str();
}


const char* FileSpec::absFileName( int fidx ) const
{
    const char* fnm = fileName( fidx );
    if ( StringView(fnm).startsWith( "${" ) )
	return fnm;

    FilePath fp( fnm );
    if ( fp.isAbsolute() )
	return fnm;

    fp.insert( fullDirName() );
    mDeclStaticString(ret); ret = fp.fullPath();
    return ret.str();
}


const char* FileSpec::dispName() const
{
    const int nrfnms = fnames_.size();
    if ( nrfnms < 2 )
	return usrStr();

    mDeclStaticString(ret); ret = fileName( 0 );
    ret.add( " (+more)" );
    return ret.str();
}


void FileSpec::setBaseDir( const char* dirnm )
{
    datadir_.set( dirnm );
}


void FileSpec::setFileName( const char* nm )
{
    setEmpty();
    if ( nm && *nm )
	fnames_.add( nm );
}


void FileSpec::ensureBaseDir( const char* dirnm )
{
    if ( !dirnm || !*dirnm )
	return;

    FilePath basefp( dirnm );
    if ( !basefp.isAbsolute() )
	return;

    const int basenrlvls = basefp.nrLevels();
    const int sz = nrFiles();
    for ( int idx=0; idx<sz; idx++ )
    {
	FilePath fp( absFileName(idx) );
	const int nrlvls = fp.nrLevels();
	if ( nrlvls <= basenrlvls )
	    fp.setPath( dirnm );
	else
	{
	    BufferString oldpath( fp.dirUpTo(basenrlvls-1) );
	    const int oldpathsz = oldpath.size();
	    BufferString oldfnm( fp.fullPath() );
	    const int oldfnmsz = oldfnm.size();
	    oldfnm[oldpathsz] = '\0';
	    fp = basefp;
	    if ( oldfnmsz > oldpathsz )
		fp.add( oldfnm.str() + oldpathsz + 1 );

	    const BufferString newfnm( fp.fullPath() );
	    if ( idx == 0 )
		setFileName( newfnm );
	    else
		*fnames_[idx] = newfnm;
	}
    }
}


void FileSpec::makeAbsoluteIfRelative( const char* dirnm )
{
    if ( !dirnm || !*dirnm )
	return;

    FilePath basefp( dirnm );
    if ( !basefp.isAbsolute() )
	return;

    const int sz = nrFiles();
    for ( int idx=0; idx<sz; idx++ )
    {
	FilePath fp( fileName(idx) );
	if ( fp.isAbsolute() )
	    continue;

	fp.setPath( dirnm );
	fnames_.replace( idx, new BufferString(fp.fullPath()) );
    }
}


void FileSpec::fillPar( IOPar& iop ) const
{
    iop.removeWithKey( sKeyFileNrs() );
    const int nrfnms = fnames_.size();
    iop.set( sKey::FileName(), nrfnms > 0 ? fnames_.get(0).buf() : "" );
    if ( nrfnms > 1 )
    {
	for ( int ifile=1; ifile<nrfnms; ifile++ )
	    iop.set( IOPar::compKey(sKey::FileName(),ifile),
		      fileName(ifile) );
    }
    else
    {
	if ( !mIsUdf(nrs_.start_) )
	{
	    FileMultiString fms;
	    fms += nrs_.start_; fms += nrs_.stop_; fms += nrs_.step_;
	    if ( zeropad_ )
		fms += zeropad_;

	    iop.set( sKeyFileNrs(), fms );
	}
    }

    if ( !usrstr_.isEmpty() )
    {
	const BufferString key( sKey::User(), " ", sKey::FileName() );
	iop.set( key, usrstr_ );
    }
}


bool FileSpec::usePar( const IOPar& iop )
{
    BufferString fnm;
    bool havemultifnames = false;
    if ( !iop.get(sKey::FileName(),fnm) )
    {
	const BufferString res = iop.find(
					IOPar::compKey(sKey::FileName(),0) );
	if ( res.isEmpty() )
	    return false;

	fnm = res;
    }

    fnames_.setEmpty();
    fnames_.add( fnm );
    havemultifnames = iop.hasKey( IOPar::compKey(sKey::FileName(),1) );

    if ( havemultifnames )
    {
	for ( int ifile=1; ; ifile++ )
	{
	    const BufferString res =
			    iop.find( IOPar::compKey(sKey::FileName(),ifile));
	    if ( res.isEmpty() )
		break;

	    fnames_.add( res );
	}
    }
    else
	getMultiFromString( iop.find(sKeyFileNrs()) );

    const BufferString key( sKey::User(), " ", sKey::FileName() );
    usrstr_.setEmpty();
    iop.get( key, usrstr_ );
    if ( File::isDirectory(datadir_.buf()) )
	makePathsRelative( datadir_.buf() );

    return true;
}


void FileSpec::getReport( IOPar& iop ) const
{
    BufferString usrstr = usrStr();
    if ( usrstr.isEmpty() )
	usrstr = fileName( 0 );
    iop.set( sKey::FileName(), usrstr );
    const int nrfnms = fnames_.size();
    const bool hasmultinrs = !mIsUdf(nrs_.start_);
    if ( nrfnms < 2 && !hasmultinrs )
	return;

    if ( nrfnms > 1 )
    {
	iop.set( "Number of additional files: ", nrfnms-1 );
	if ( nrfnms == 2 )
	    iop.set( "Additional file: ", fileName(1) );
	else
	{
	    iop.set( "First additional file: ", fileName(1) );
	    iop.set( "Last additional file: ", fileName(nrfnms-1) );
	}
    }
    else
    {
	BufferString str;
	str += nrs_.start_; str += "-"; str += nrs_.stop_;
	str += " step "; str += nrs_.step_;
	if ( zeropad_ )
	    { str += "(pad to "; str += zeropad_; str += " zeros)"; }
	iop.set( "Replace '*' with", str );
    }
}


void FileSpec::makePathsRelative( IOPar& iop, const char* dir )
{
    FileSpec fs; fs.usePar( iop );
    fs.makePathsRelative( dir );
    fs.fillPar( iop );
}


void FileSpec::makePathsRelative( const char* dir )
{
    const int nrfnms = fnames_.size();
    if ( nrfnms < 1 )
	return;

    if ( !dir || !*dir )
    {
	if ( datadir_.isEmpty() )
	    { pErrMsg("Should not be reached"); }

	dir = datadir_;
    }

    const FilePath relfp( dir );
    bool hasrelativepaths = false;
    for ( int ifile=0; ifile<nrfnms; ifile++ )
    {
	const BufferString fnm( fileName(ifile) );
	if ( fnm.isEmpty() )
	    continue;

	FilePath fp( fnm );
	if ( fp.isSubDirOf(relfp) )
	{
	    BufferString relpath = File::getRelativePath( relfp.fullPath(),
							  fp.pathOnly() );
	    if ( !relpath.isEmpty() )
	    {
		FilePath newrelfp( relpath, fp.fileName() );
		relpath = newrelfp.fullPath();
		if ( relpath != fnm )
		{
		    fnames_.get(ifile).set( relpath );
		    hasrelativepaths = true;
		}
	    }
	}
    }

    if ( datadir_.isEmpty() && hasrelativepaths )
	datadir_.set( dir );
}


void FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start_ = len > 0 ? fms.getIValue( 0 ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop_ = fms.getIValue( 1 );
    if ( len > 2 )
	nrs_.step_ = fms.getIValue( 2 );
    if ( len > 3 )
	zeropad_ = fms.getIValue( 3 );
}
