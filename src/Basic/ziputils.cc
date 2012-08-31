/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: ziputils.cc,v 1.18 2012-08-31 05:28:14 cvssalil Exp $";

#include "ziputils.h"


#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
    { \
	errmsg_ = dir; \
	errmsg_ += " does not exist"; \
	return false; \
    } \



ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_( filelistnm )
    , needfilelist_( !filelistname_.isEmpty() )
{}

bool ZipUtils::Zip( const char* src, const char* dest )
{
   mDirCheck( src );
   mDirCheck( dest );

#ifdef __win__
    return doZip( src, dest );
#else
    // TODO on Linux
    return false;
#endif
}

bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck( src );
    mDirCheck( dest );
    return doUnZip( src, dest );
}

bool ZipUtils::doZip( const char* src, const char* dest )
{
    return true;
}

bool ZipUtils::doUnZip( const char* src, const char* dest )
{
    bool tempfile = false;
    FilePath orgfnm( filelistname_ );
    if ( needfilelist_ )
    {
	if ( !File::exists( orgfnm.pathOnly() ) ) 
	{
	    tempfile = true;
	    FilePath listfp( src );
	    if (  listfp.nrLevels() <= 1 )
		filelistname_ = orgfnm.fileName();
	    else
	    {
		listfp = listfp.pathOnly();
		listfp.add( orgfnm.fileName() );
		filelistname_ = listfp.fullPath();
	    }
	}    
    }

    bool res = false;
#ifdef __win__
    BufferString cmd( "cmd /c unzip -o \"", src );
    cmd.add( "\" -d \"" ).add( dest ).add( "\"");
    if ( needfilelist_ )
	cmd.add( " > " ).add( "\"" ).add( filelistname_ ).add( "\"" );
    res = ExecOSCmd( cmd );
#else
    BufferString cmd( "unzip -o ", src );
    cmd.add( " -d " ).add( dest ).add( " > " )
	.add( needfilelist_ ? filelistname_ : "/dev/null" );
    res = !system( cmd );
#endif

    if ( res && tempfile )
    {
	File::copy( filelistname_, orgfnm.fullPath() );
	File::remove( filelistname_ );
	return true;
    }

    errmsg_ = !res ? " Unzip Failed" : ""; 
    return res;
}

bool ZipUtils::zCompress ( BufferString& src, TaskRunner* tr )
{
    FilePath fp = src;
    ziphdler_.nrlevel_ = fp.nrLevels();
    ziphdler_.destfile_ = src;
    ziphdler_.destfile_.add( ".zip" );
    StreamData osd = StreamProvider( ziphdler_.destfile_.buf() ).makeOStream( true );
    osd.ostrm;
    std::ostream& dest = *osd.ostrm;
    if ( !osd.usable() ) 
   {
	ziphdler_.errormsg_ = "Output stream not working";
	return false;
   }
    if ( File::isDirectory( src.buf() ) == true )
    {
	ziphdler_.allfiles_.add( src );
	ziphdler_.dirManage( src.buf(), dest );
	ziphdler_.totalfiles_ = ziphdler_.allfiles_.size();
    }
    else if ( File::isFile( src.buf() ) == true )
    {
	ziphdler_.totalfiles_ = 1;
	ziphdler_.allfiles_.add( src );
    }
    else
    {
	ziphdler_.errormsg_ = src;
	ziphdler_.errormsg_.add( " does not exist\0" );
	return false;
    }

    Zipping exec(ziphdler_.allfiles_, dest, ziphdler_, tr );
    tr ? tr->execute( exec ) : exec.execute();
    osd.close();
    return 1;
}

int Zipping::nextStep()
{
    const bool ret = ziphd_.openStrmToRead( flist_.get(nrdone_), dest_ );
    nrdone_++;
    if ( nrdone_ < totalNr() )
	return MoreToDo();
    else
    {
	int ptrlctn = dest_.tellp();
	ziphd_.setCntrlDirHeader( dest_ );
	ziphd_.setEndOfCntrlDirHeader ( dest_, ptrlctn );
	return Finished();
    }
}

od_int64 Zipping::nrDone() const
{
    return nrdone_;
}

od_int64 Zipping::totalNr() const
{
    return flist_.size();
}

const char* Zipping::nrDoneText() const
{
    return ( "Files" );
}


bool ZipUtils::zUnCompress( BufferString& srcfnm, TaskRunner* tr )
{
    FilePath fp;
    if ( !File::exists( srcfnm.buf() ) )
    {
	ziphdler_.errormsg_ = srcfnm; 
	ziphdler_.errormsg_ += " does not exist"; 
	return false; 
    }
    ziphdler_.srcfile_ = srcfnm;
    fp = srcfnm;
    ziphdler_.destbasepath_ = fp.pathOnly();
    ziphdler_.destbasepath_ += fp.dirSep( fp.Local );
    StreamData isd = StreamProvider( srcfnm ).makeIStream();
    isd.istrm;
    std::istream& src = *isd.istrm;
    if ( !isd.usable() ) 
    {
	ziphdler_.errormsg_ = "Input stream not working";
	return false;
    }
    ziphdler_.readEndOfCntrlDirHeader( src );
    UnZipping exec( src, ziphdler_, tr );
    tr ? tr->execute( exec ) : exec.execute();
    isd.close();
    return true;
}

bool ZipUtils::zUnCompress( BufferString& srcfnm, BufferString& fnm, 
							TaskRunner* tr )
{
    FilePath fp;
    int ret;
    if ( !File::exists( srcfnm.buf() ) )
    {
	ziphdler_.errormsg_ = fnm; 
	ziphdler_.errormsg_ += " does not exist"; 
	return false; 
    }
    ZipArchiveInfo zai( srcfnm );
    unsigned int offset = zai.getLocalHeaderOffset( fnm );
    StreamData isd = StreamProvider( srcfnm ).makeIStream();
    isd.istrm;
    std::istream& src = *isd.istrm;
    if ( !isd.usable() ) 
    {
	ziphdler_.errormsg_ = "Input stream not working";
	return false;
    }
    src.seekg( offset );
    ziphdler_.srcfile_ = srcfnm;
    fp = fnm;
    ziphdler_.destbasepath_ = fp.pathOnly();
    ziphdler_.destbasepath_ += fp.dirSep( fp.Local );
    ziphdler_.readFileHeader( src );
    StreamData osd = StreamProvider( ziphdler_.destfile_ ).makeIStream();
    osd.ostrm;
    std::ostream& dest = *osd.ostrm;
    if ( !osd.usable() ) 
    {
	ziphdler_.errormsg_ = "Output stream not working";
	return false;
    }
    if ( ziphdler_.compmethod_ == 8 )
	    ret = ziphdler_.doZUnCompress( src, dest );
    else if ( ziphdler_.compmethod_ == 0 )
    {
	unsigned char* in = new unsigned char[ziphdler_.srcfilesize_];
	src.read( (char*) in, ziphdler_.srcfilesize_ );
	dest.write ( (char*) in, ziphdler_.destfilesize_ );
	delete [] in;
	ziphdler_.setTimeDateModified( ziphdler_.lastmodtime_, 
					ziphdler_.lastmoddate_ );
    }
    else
    {
	ziphdler_.errormsg_ = "Compression method::not supported";
	return false;
    }
    osd.close();
    ziphdler_.setTimeDateModified( ziphdler_.lastmodtime_, 
					    ziphdler_.lastmoddate_ );
    isd.close();
    return 1;
}


int UnZipping::nextStep()
{
    int ret = ziphd_.readFileHeader( src_ );
    if ( ret == 1 )
    {
	StreamData osd = StreamProvider( ziphd_.destfile_ ).makeOStream();
	osd.ostrm;
	std::ostream& dest = *osd.ostrm;
	if ( !osd.usable() ) 
	{
	    ziphd_.errormsg_ = "Input stream not working";
	    return ErrorOccurred();
	}
	if ( ziphd_.compmethod_ == 8 )
	    ret = ziphd_.doZUnCompress( src_, dest );
	else if ( ziphd_.compmethod_ == 0 )
	{
	    unsigned char* in = new unsigned char[ziphd_.srcfilesize_];
	    src_.read( (char*) in, ziphd_.srcfilesize_ );
	    dest.write ( (char*) in, ziphd_.destfilesize_ );
	    delete [] in;
	    ziphd_.setTimeDateModified( ziphd_.lastmodtime_, 
					ziphd_.lastmoddate_ );
	}
	else
	{
	    ziphd_.errormsg_ = "Compression method::not supported";
	    return ErrorOccurred();
	}
	osd.close();
	ziphd_.setTimeDateModified( ziphd_.lastmodtime_, ziphd_.lastmoddate_ );
    }
    else if ( ret == -1)
    {
    }
    if ( ret == 0)
	return ErrorOccurred();
    else
    {
	nrdone_++;
	if ( nrDone() < totalNr() )
	    return MoreToDo();
	else
	    return Finished();
    }
}

od_int64 UnZipping::nrDone() const
{
    return nrdone_;
}

od_int64 UnZipping::totalNr() const
{
    return ziphd_.totalfiles_;
}

const char* UnZipping::nrDoneText() const
{
    return ( "Files" );
}

const char* UnZipping::message() const
{
    return ziphd_.errorMsg();
}

const char* ZipUtils::errorMsg()
{
    return ziphdler_.errorMsg();
}