/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Salil Agarwal
Date:          30 August 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ziphandler.h"

#include "bufstring.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "executor.h"
#include "separstr.h"
#include "task.h"
#include "varlenarray.h"
#include "ziparchiveinfo.h"

#include "QFileInfo"
#include "QDateTime"
#include "QDate"
#include "QTime"

#ifdef HAS_ZLIB
#include "zlib.h"
#endif

#ifdef __win__
#include "sys/utime.h"
#else
#include "utime.h"
#endif

#include <iostream>
#include <fstream>
#include <math.h>

#define mVerNeedToExtract 20
#define mDeflate 8
#define mLVerNeedToExtract 4		//L for Location
#define mLGenPurBitFlag 6
#define mLCompMethod 8
#define mLLastModFTime 10
#define mLLastModFDate 12
#define mLCRC32 14
#define mLCompSize 18
#define mLUnCompSize 22
#define mLFnmLength 26
#define mLExtraFldLength 28
#define mLVerMadeBy 4
#define mLCRCCentral 16
#define mLCompSizeCentral 20
#define mLUnCompSizeCentral 24
#define mLFnmLengthCentral 28
#define mLExtraFldLengthCentral 30
#define mLFileComntLength 32
#define mLDiskNoStart 34
#define mLIntFileAttr 36
#define mLExtFileAttr 38
#define mLRelOffset 42
#define mLSizeOfData 4
#define mLDiskNo 4
#define mLCentralDirDiskNo 6
#define mLCentralDirVersion 6
#define mLCentralDirBitFlag 8
#define mLCentralDirCompMethod 10
#define mLTotalEntryOnDisk 8
#define mLTotalEntry 10
#define mLSizeCentralDir 12
#define mLOffsetCentralDir 16
#define mLZipFileComntLength 20
#define mSizeTwoBytes 2
#define mSizeFourBytes 4
#define mMaxChunkSize 10485760	    //10MB

#define mZDefaultMemoryLevel 9
#define mMaxWindowBitForRawDeflate -15

#define mWriteErr(filenm) \
    errormsg_ = "Unable to write to "; \
    errormsg_ += filenm; \
    return false;

#define mReadErr(filenm) \
    errormsg_ = "Unable to read from "; \
    errormsg_ += filenm; \
    return false;

#define mErrRet(pre,mid,post) \
    errormsg_ = pre; \
    errormsg_ += mid; \
    errormsg_ += post; \
    return false;


ZipHandler::~ZipHandler()
{ delete ziparchinfo_; }


bool ZipHandler::initMakeZip( const char* destfnm, BufferStringSet srcfnms )
{
    for ( int idx=0; idx<srcfnms.size(); idx++ )
    {
	if ( File::isFile(srcfnms.get(idx).buf()) )
	{
	    allfilenames_.add( srcfnms.get(idx) );
	    cumulativefilecounts_ += allfilenames_.size();
	}
	else if ( File::isDirectory(srcfnms.get(idx).buf()) )
	{
	    allfilenames_.add( srcfnms.get(idx) );
	    getFileList( srcfnms.get(idx).buf(), allfilenames_ );
	    cumulativefilecounts_ += allfilenames_.size();
	}
	else
	{ mErrRet( srcfnms.get(idx), " does not exist.", "" ) }
    }

    FilePath fp( srcfnms.get(0).buf() );
    curnrlevels_ = fp.nrLevels();
    initialfilecount_ = 0;
    destfile_ = destfnm;
    osd_ = StreamProvider( destfile_.buf() ).makeOStream( true );
    if ( !osd_.usable() )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::getFileList( const char* src, 
			      BufferStringSet& filenames ) const
{

    DirList dlist( src, DirList::DirsOnly, 0 );
    DirList flist( src, DirList::FilesOnly, 0 );

    for( int idx=0; idx<dlist.size(); idx++ )
    {
	filenames.add( dlist.fullPath(idx) );
	getFileList( dlist.fullPath(idx), filenames);
    }

    for( int idx=0; idx<flist.size(); idx++)
	filenames.add( flist.fullPath(idx) );

    return true;
}



bool ZipHandler::compressNextFile()
{
    if ( curfileidx_ == getCumulativeFileCount(curinputidx_) )
    {
	curinputidx_++;
	FilePath fp( allfilenames_.get(curfileidx_) );
	curnrlevels_ =  fp.nrLevels();
    }

    int ret;
    ret = openStrmToRead( allfilenames_.get(curfileidx_) );
    if ( ret == 0 )
    {
	osd_.close();
	return false;
    }
	
    if ( ret == 2 )
	ret = setLocalFileHeaderForDir();
    else if ( ret == 1 )
    {
	ret = doZCompress();
	isd_.close();

    }

    if ( !ret )
    {
	osd_.close();
	return false;
    }

    curfileidx_++;
    return true;
}


int ZipHandler::openStrmToRead( const char* src )
{
    srcfile_ = src;
    if ( File::isDirectory(src) )
	return 2;

    if( !File::exists(src) )
    { 
	errormsg_ = src;
	errormsg_ += " does not exist.";
	return 0;
    }

    isd_ = StreamProvider( src ).makeIStream( true );
    if ( !isd_.usable() ) 
    { 
	errormsg_ = "Unable to read from ";
	errormsg_ += src;
	return 0;
    }

    return 1;
}


void ZipHandler::setCompLevel( CompLevel cl )
{ complevel_ = cl; }


bool ZipHandler::doZCompress()
{
#ifdef HAS_ZLIB
    const od_uint32 ptrlocation = osd_.ostrm->tellp();
    if ( !setLocalFileHeader( ) )
	return false;

    int ret;
    z_stream zlibstrm;
    const int method = Z_DEFLATED;
    const int windowbits = mMaxWindowBitForRawDeflate;
    const int memlevel = mZDefaultMemoryLevel;
    const int stategy = Z_DEFAULT_STRATEGY;
    zlibstrm.zalloc = Z_NULL;
    zlibstrm.zfree = Z_NULL;
    zlibstrm.opaque = Z_NULL;
    ret = deflateInit2( &zlibstrm, complevel_, method, windowbits, memlevel,
								    stategy );
    const od_uint32 chunksize = mMIN( mMaxChunkSize, srcfilesize_ );
    od_uint32 upperbound = deflateBound( &zlibstrm, chunksize );
    if ( ret != Z_OK ) 
    {
	mErrRet( "Error Details:Initialization required to compress data \
		fails.\n", "Error type:", ret )
    }

    od_uint32 destfilesize = 0;
    crc_ = 0;

    int flushpolicy = Z_FINISH;
    mAllocVarLenArr( unsigned char, in, chunksize );
    mAllocVarLenArr( unsigned char, out, upperbound );
    if ( !in.ptr() || !out.ptr()  )
    { mErrRet( "Unable to allot memory on the heap","","") }

    od_uint32 bytestowrite;
    do
    {
	isd_.istrm->read( mCast(char*,in.ptr()), chunksize );
	zlibstrm.avail_in = isd_.istrm->gcount();
	flushpolicy = isd_.istrm->eof() ? Z_FINISH : Z_NO_FLUSH;
	crc_ = crc32( crc_, mCast(Bytef*,in.ptr()), isd_.istrm->gcount() );
	zlibstrm.next_in = in.ptr();
	do
	{
	    zlibstrm.avail_out = upperbound;
	    zlibstrm.next_out = out.ptr();
	    ret = deflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 )
	    {
		mErrRet( "Failed to zip ", srcfile_, "\nFile may contain \
						     corrupt data" )
	    }

	    bytestowrite = upperbound - zlibstrm.avail_out;
	    destfilesize = destfilesize + bytestowrite;
	    if ( bytestowrite > 0 ) 
		osd_.ostrm->write( mCast(const char*,out.ptr()), bytestowrite );

	    if ( !*osd_.ostrm )
	    {
		(void) deflateEnd ( &zlibstrm );
		mWriteErr( destfile_ )
	    }

	} while ( zlibstrm.avail_out == 0 );

    } while( flushpolicy != Z_FINISH );

    deflateEnd( &zlibstrm );
    osd_.ostrm->seekp( ptrlocation + mLCRC32 );
    osd_.ostrm->write( mCast(const char*,&crc_), sizeof(od_uint32) );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    osd_.ostrm->write( mCast(const char*,&destfilesize), sizeof(od_uint32) );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    osd_.ostrm->seekp( mHeaderSize + destfilesize
			    			+ srcfnmsize_ + ptrlocation );
    return true;
#else
    pErrMsg( "ZLib library not available" );
    return false;
#endif
}


bool ZipHandler::setLocalFileHeader()
{
    srcfilesize_ = mCast( od_uint32, File::getFileSize(srcfile_) );
    FilePath fnm( srcfile_ );
    int p = fnm.nrLevels();
    BufferString srcfnm = "";
    for ( int idx = ( curnrlevels_ - 1 ); idx <= (p - 2); idx++ )
    {
	srcfnm.add( fnm.dir( idx ) );
	srcfnm += "/";
    }

    srcfnm.add( fnm.fileName() );
    srcfnmsize_ = ( od_uint16 ) srcfnm.size();
    char* buf = 0;
    unsigned char headerbuff[1024];
    const od_uint16 nullvalue = 0;
    const od_uint16 version = mVerNeedToExtract;
    const od_uint16 compmethod = mDeflate;

    mLocalFileHeaderSig ( headerbuff );
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtract, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLGenPurBitFlag, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, compmethod, mLCompMethod, mSizeTwoBytes );
    switch ( complevel_ )
    {
    case Maximum:
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
    case Fast:
	setBitValue( headerbuff[mLGenPurBitFlag], 3, 1 );
    case SuperFast:
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
	setBitValue( headerbuff[mLGenPurBitFlag], 3, 1 );
    case NoComp:
	mInsertToCharBuff( headerbuff, nullvalue, mLCompMethod, mSizeTwoBytes );
    }

    const od_uint16 dostime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dostime, mLLastModFTime, mSizeTwoBytes );
    const od_uint16 dosdate = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dosdate, mLLastModFDate, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, srcfilesize_, mLUnCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtraFldLength, mSizeTwoBytes );
    osd_.ostrm->write( mCast(const char*,headerbuff), mHeaderSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    osd_.ostrm->write( srcfnm.buf(), srcfnmsize_ );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::setLocalFileHeaderForDir()
{
    FilePath fnm( srcfile_ );
    int p = fnm.nrLevels();
    BufferString srcfnm = "";
    for ( int idx=(curnrlevels_-1); idx<=(p-1); idx++ )
    {
	srcfnm.add( fnm.dir( idx ) );
	srcfnm += "/";
    }

    srcfnmsize_ = ( od_uint16 ) srcfnm.size();
    unsigned char headerbuff[1024];
    mLocalFileHeaderSig ( headerbuff );
    headerbuff[mLVerNeedToExtract] = mVerNeedToExtract;
    for ( int idx=5; idx<26; idx++ )
	headerbuff[idx] = '\0';

    char* buf = 0;
    const od_uint16 nullvalue = 0;
    const od_uint16 dostime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dostime, mLLastModFTime, mSizeTwoBytes );
    const od_uint16 dosdate= dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dosdate, mLLastModFDate, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtraFldLength, mSizeTwoBytes );
    osd_.ostrm->write( mCast(const char*,headerbuff), mHeaderSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    osd_.ostrm->write( srcfnm.buf(), srcfnmsize_ );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::setEndOfArchiveHeaders()
{
    const int ptrlctn = (int) osd_.ostrm->tellp();
    bool ret = setCentralDirHeader();
    if ( !ret )
    {
	osd_.close();
	return false;
    }

    ret = setEndOfCentralDirHeader ( ptrlctn );
    osd_.close();
    return ret;
}


bool ZipHandler::setCentralDirHeader()
{
    StreamData readdest = StreamProvider( destfile_.buf() ).makeIStream( true );
    if ( !readdest.usable() )
    { mReadErr( destfile_ ) }

    char headerbuff[1024];
    char* buf;
    const od_uint32 nullvalue = 0;
    const od_uint32 zipversion = 63;	    //Zip version used is 6.3

    mCntrlDirHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, zipversion, mLVerMadeBy, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLFileComntLength, 
		       mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNoStart, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLIntFileAttr, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtFileAttr, mSizeFourBytes );
    for( int index=0; index<cumulativefilecounts_.last()+initialfilecount_; 
								index++ )
    {
	od_uint32 offset;
	od_uint32 compsize;
	od_uint16 fnmsize;
	char localheader[1024];
	offset = readdest.istrm->tellg();
	readdest.istrm->read( mCast(char*,localheader), mHeaderSize);
	if ( !*readdest.istrm )
	{ mReadErr( destfile_ ) }

	localheader[mHeaderSize] = 0;
	compsize = *(od_uint32*)( localheader + mLCompSize );
	fnmsize = *(od_uint16*)( localheader + mLFnmLength );
	readdest.istrm->read( mCast(char*,localheader+mHeaderSize),fnmsize );
	localheader[mHeaderSize + fnmsize] = 0;
	for( int id=mSizeFourBytes; id<mHeaderSize; id++ )
	    headerbuff[id + 2] = localheader[id];

	for( int id=0; id<fnmsize; id++ )
	    headerbuff[id + mCentralHeaderSize] = localheader[id + mHeaderSize];

	mInsertToCharBuff( headerbuff, offset, mLRelOffset, mSizeFourBytes );
	readdest.istrm->seekg( mHeaderSize + compsize + fnmsize + offset );
	osd_.ostrm->write( mCast(char*,headerbuff),mCentralHeaderSize+fnmsize );
	if ( !*osd_.ostrm )
	{ mWriteErr( destfile_ ) }

    } 

    readdest.close();
    return true;
}


bool ZipHandler::setEndOfCentralDirHeader( int ptrlctn )
{
    char headerbuff[100];
    mEndOfCntrlDirHeaderSig( headerbuff );
    const od_uint32 nullvalue = 0;
    char* buf;
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCentralDirDiskNo,mSizeTwoBytes);
    int ptrlocation = osd_.ostrm->tellp();
    int sizecntrldir = ptrlocation - ptrlctn;

    cumulativefilecounts_[(cumulativefilecounts_.size()-1)] = 
			  cumulativefilecounts_.last() + initialfilecount_;
    mInsertToCharBuff( headerbuff, cumulativefilecounts_.last(), 
		       mLTotalEntryOnDisk, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, cumulativefilecounts_.last(), mLTotalEntry, 
		       mSizeTwoBytes);
    mInsertToCharBuff( headerbuff, sizecntrldir, mLSizeCentralDir, 
		       mSizeFourBytes );
    mInsertToCharBuff( headerbuff, ptrlctn, mLOffsetCentralDir, 
		       mSizeFourBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLZipFileComntLength, 
		       mSizeTwoBytes );
    osd_.ostrm->write( mCast(char*,headerbuff), mEndOfDirHeaderSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::initAppend( const char* srcfnm, const char* fnm )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    FilePath fp( fnm );
    curnrlevels_ = fp.nrLevels();
    destfile_ = srcfnm;
    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    { mReadErr( srcfnm ) }

    if ( !readEndOfCentralDirHeader() )
	return false;

    initialfilecount_ = cumulativefilecounts_.last();
    isd_.close();
    if ( File::isFile( fnm ) )
    {
	cumulativefilecounts_[ (cumulativefilecounts_.size()-1) ] = 1;
	allfilenames_.add( fnm );
    }
    else if ( File::isDirectory( fnm ) )
    {
	allfilenames_.add( fnm );
	getFileList( fnm, allfilenames_ );
	cumulativefilecounts_[ (cumulativefilecounts_.size()-1) ] = 
							   allfilenames_.size();
    }
    else
    { mErrRet( fnm, " does not exist", "") }

    osd_ = makeOStreamForAppend( srcfnm );
    if ( !osd_.usable() )
    { mWriteErr( srcfnm ) }

    osd_.ostrm->seekp( offsetofcentraldir_ );
    return true;
}


bool ZipHandler::getArchiveInfo( const char* srcfnm, 
				 ObjectSet<ZipFileInfo>& zfileinfo )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    { mReadErr( srcfnm ) }

    if ( !readEndOfCentralDirHeader() )
	return false;

    isd_.istrm->seekg( offsetofcentraldir_, std::ios::beg );
    od_uint32 ptrlocation;
    ptrlocation = isd_.istrm->tellg();
    char headerbuff[1024];
    bool sigcheck;
    for ( int idx=0; idx<cumulativefilecounts_.last(); idx++ )
    {
	isd_.istrm->read( mCast(char*,headerbuff), mCentralHeaderSize);
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    isd_.close();
	    mErrRet( srcfnm, " is not a valid zip archive", "" )
	}

	if ( getBitValue( *(headerbuff + mLCentralDirBitFlag), 0 ) )
	{
	    mErrRet( "Encrypted file::Not supported", "", "" )
	    ////TODO
	}

	od_uint16 version = *mCast( od_uint16*, headerbuff + 
							mLCentralDirVersion );
	if ( version > mVerNeedToExtract )
	{ mErrRet("Failed to unzip ", srcfnm, 
		   "\nVersion of zip format needed to unpack is not supported")}

	od_uint16 compmethod = *mCast( od_uint16*, headerbuff + 
						    mLCentralDirCompMethod );
	if ( compmethod != mDeflate && compmethod != 0 )
	{ mErrRet( "Failed to unzip ", srcfnm, "\nCompression method used \
						   is not supported" ) }

	od_uint16 bitflag = *mCast( od_uint16*, headerbuff + 
						    mLCentralDirBitFlag );
	if ( bitflag > 14 )
	{ mErrRet("Failed to unzip ", srcfnm, 
		   "\nVersion of zip format needed to unpack is not supported")}

	isd_.istrm->seekg( ptrlocation + mCentralHeaderSize );
	BufferString headerfnm;
	isd_.istrm->read( headerfnm.buf(),
			    *mCast(od_int16*,headerbuff+mLFnmLengthCentral) );
	headerfnm[*((od_int16*)(headerbuff+mLFnmLengthCentral))] = '\0';

	ZipFileInfo* fi = new ZipFileInfo( headerfnm, 
			    *mCast(od_uint32*,headerbuff+mLCompSizeCentral),
			    *mCast(od_uint32*,headerbuff+mLUnCompSizeCentral),
			    *mCast(od_uint32*,headerbuff+mLRelOffset) );
	zfileinfo += fi;
	ptrlocation = ptrlocation
			+ *mCast( od_int16*,headerbuff+mLFnmLengthCentral )
			+ *mCast( od_int16*,headerbuff+mLExtraFldLengthCentral )
			+ *mCast( od_int16*,headerbuff+mLFileComntLength )
			+ mCentralHeaderSize;
	isd_.istrm->seekg( ptrlocation );
    }

    isd_.istrm->read( mCast(char*,headerbuff), 10);
    isd_.close();
    return true;
}


bool ZipHandler::initUnZipArchive( const char* srcfnm, const char* basepath )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    ziparchinfo_ = new ZipArchiveInfo( srcfnm );
    srcfile_ = srcfnm;
    if ( !File::isDirectory(basepath) && !File::createDir(basepath) )
    { mErrRet( basepath, " is not a valid path", "" ) }
    
    FilePath destpath( basepath );
    destbasepath_ = destpath.fullPath();
    destbasepath_ += FilePath::dirSep( FilePath::Local );
    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    { mReadErr( srcfnm ) }

    if ( !readEndOfCentralDirHeader() )
    {
	isd_.close();
	return false;
    }

    return true;
}


bool ZipHandler::unZipFile( const char* srcfnm, const char* fnm, 
			    const char* path )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    ZipArchiveInfo zai( srcfnm );
    od_int64 offset = zai.getLocalHeaderOffset( fnm );
    if ( offset == -1 )
    {
	errormsg_ = zai.errorMsg();
	return false;
    }

    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() ) 
    { mReadErr( srcfnm ) }

    if ( !readEndOfCentralDirHeader() )
    {
	isd_.close();
	return false;
    }

    isd_.istrm->seekg( offset );
    srcfile_ = srcfnm;
    FilePath fp;
    fp = srcfnm;
    if ( !File::isDirectory(path) && !File::createDir(path) )
    { mErrRet( path, " is not a valid path", "" ) }

    FilePath destpath( path );
    destbasepath_ = destpath.fullPath();
    destbasepath_ += FilePath::dirSep( FilePath::Local );
    if ( !extractNextFile() )
	return false;

    if ( curfileidx_ == cumulativefilecounts_.last() )
	return true;

    isd_.close();
    return true;
}


bool ZipHandler::readEndOfCentralDirHeader()
{
    char headerbuff[mEndOfDirHeaderSize];
    int ptrlocation;
    char sig[mSizeFourBytes];
    mEndOfCntrlDirHeaderSig( sig );
    isd_.istrm->seekg( 0 , std::ios::end );
    ptrlocation = isd_.istrm->tellg();
    if ( ptrlocation == 0 )
    { mErrRet( "Zip archive is empty", "", "" ) }

    isd_.istrm->seekg( ptrlocation - mEndOfDirHeaderSize , std::ios::beg );
    ptrlocation = isd_.istrm->tellg();
    isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    while (!( *mCast(od_uint32*,headerbuff) == *mCast(od_uint32*,sig) ))
    {
	isd_.istrm->seekg( (ptrlocation-1), std::ios::beg );
	ptrlocation = isd_.istrm->tellg();
	if ( ptrlocation == 0 || ptrlocation == -1 )
	{ mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

	isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    }

    isd_.istrm->read( mCast(char*,headerbuff+mSizeFourBytes),
				        mEndOfDirHeaderSize-mSizeFourBytes );
    isd_.istrm->seekg(0);   
    cumulativefilecounts_ += *mCast( od_int16*, headerbuff+mLTotalEntry );
    offsetofcentraldir_ = *mCast( int*, headerbuff+mLOffsetCentralDir );
    return true;
}


bool ZipHandler::extractNextFile()
{

    const int ret = readLocalFileHeader();
    if ( ret == 2 )
    {
	curfileidx_++;
	if ( curfileidx_ == cumulativefilecounts_.last() )
	    isd_.close();

	return true;
    }
    
    if ( ret == 0 )
    {
	isd_.close();
	return false;
    }

    if ( !openStreamToWrite() )
    {
	isd_.close();
	return false;
    }

    if ( compmethod_ == mDeflate )
    {
	if ( !doZUnCompress() )
	{
	    osd_.close();
	    isd_.close();
	    return false;
	}
    }
    else if ( compmethod_ == 0 )
    {
	const od_uint32 chunksize = mMIN( mMaxChunkSize, srcfilesize_ );
	int count = chunksize;
	bool finish = false;
	mAllocVarLenArr( unsigned char, in, chunksize );
	if ( !in.ptr() )
	{ mErrRet( "Unable to allot memory on the heap","","" ) }

	do
	{
	    if ( count <= srcfilesize_ )
	    {
		isd_.istrm->read( (char*) in.ptr(), chunksize );
		osd_.ostrm->write ( (char*) in.ptr(), chunksize );
	    }
	    else
	    {
		isd_.istrm->read( (char*) in.ptr(), srcfilesize_ % chunksize );
		osd_.ostrm->write ( (char*) in.ptr(), srcfilesize_ % chunksize);
		finish = true;
	    }

	    count += chunksize;
	    if ( !*osd_.ostrm || !*isd_.istrm )
	    {
		isd_.close();
		osd_.close();
		mErrRet( "Failed to unzip ", srcfile_, "\nError occured while \
						   writing to disk" )
	    }
	}while ( finish == false );

    }
    else
    {
	osd_.close();
	isd_.close();
	mErrRet( "Failed to unzip ", srcfile_, "\nCompression method used \
					       is not supported" )
    }

    osd_.close();
    setTimeDateModified( destfile_.buf(), lastmodtime_, lastmoddate_ );
    curfileidx_++;
    if ( curfileidx_ == cumulativefilecounts_.last() )
	isd_.close();

    return true;
}


int ZipHandler::readLocalFileHeader()
{
    char headerbuff[1024];
    const od_int64 ptrlocation = isd_.istrm->tellg();
    isd_.istrm->read( (char*) headerbuff, mHeaderSize );
    headerbuff[mHeaderSize] = 0;
    if ( isd_.istrm->gcount() != mHeaderSize )
    { mReadErr( srcfile_ ) }

    bool sigcheck;
    mFileHeaderSigCheck( headerbuff, 0 );
    if ( !sigcheck )
    { mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

    if ( getBitValue( *(headerbuff + mLGenPurBitFlag), 0 ) )
    {
	mErrRet( "Encrypted file::Not supported", "", "" )
	    ////TODO
    }

    od_uint16 version = *mCast( od_uint16*, headerbuff + mLVerNeedToExtract );
    if ( version > mVerNeedToExtract )
    { mErrRet("Failed to unzip ", srcfile_, 
	      "\nVersion of zip format needed to unpack is not supported")}

    od_uint16 compmethod = *mCast( od_uint16*, headerbuff + mLCompMethod );
    if ( compmethod != mDeflate && compmethod != 0 )
    { mErrRet( "Failed to unzip ", srcfile_, "\nCompression method used \
					   is not supported" ) }

    od_uint16 bitflag = *mCast( od_uint16*, headerbuff + mLGenPurBitFlag );
    if ( bitflag > 14 )
    { mErrRet("Failed to unzip ", srcfile_, 
	      "\nVersion of zip format needed to unpack is not supported")}

    compmethod_ = *mCast( od_uint16*, headerbuff + mLCompMethod );
    lastmodtime_ = *mCast( od_uint16*, headerbuff + mLLastModFTime );
    lastmoddate_ = *mCast( od_uint16*, headerbuff + mLLastModFDate );
    crc_ = *mCast( od_uint32*, headerbuff + mLCRC32 );
    srcfilesize_ = *mCast( od_uint32*, headerbuff + mLCompSize );
    srcfnmsize_ = *mCast( od_uint16*, headerbuff + mLFnmLength );
    od_uint16 xtrafldlth = *mCast( od_uint16*, headerbuff + mLExtraFldLength );
    isd_.istrm->read( mCast(char*,headerbuff), srcfnmsize_ );
    if ( isd_.istrm->gcount() != srcfnmsize_ )
    { mReadErr( srcfile_ ) }

    headerbuff[srcfnmsize_] = 0;
    if ( headerbuff[srcfnmsize_ - 1] == '/' )
    {
	headerbuff[srcfnmsize_ - 1] = 0;
	destfile_ = destbasepath_;
	destfile_ += headerbuff;
	if ( !File::exists(destfile_.buf()) && 
	     !File::createDir(destfile_.buf()) )
	{ mErrRet("Failed to unzip ",srcfile_,"\nUnable to create directory.") }

	isd_.istrm->seekg(ptrlocation + srcfnmsize_ + xtrafldlth + mHeaderSize);
	return 2;
    }

    destfile_ = destbasepath_;
    destfile_ += headerbuff;
    isd_.istrm->seekg( ptrlocation + srcfnmsize_ + xtrafldlth + mHeaderSize );
    return 1;
}


bool ZipHandler::openStreamToWrite()
{
    SeparString str( destfile_.buf(), '/' );
    BufferString pathonly = 0;
    FilePath fp = destfile_.buf();
    if ( str.size() == 1 )
	pathonly = fp.pathOnly();    
    else
    {
	for ( int idx=0; idx<str.size()-1; idx++ )
	{
	    pathonly += str[idx];
	    pathonly += fp.dirSep( fp.Local ); 
	}

    }

    if ( !File::exists( pathonly.buf() ) )
	File::createDir( pathonly.buf() );

    osd_ = StreamProvider( destfile_ ).makeOStream();
    if ( !osd_.usable() ) 
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::doZUnCompress()
{
#ifdef HAS_ZLIB
    z_stream zlibstrm;
    const int windowbits = -15;
    zlibstrm.zalloc = Z_NULL;
    zlibstrm.zfree = Z_NULL;
    zlibstrm.opaque = Z_NULL;
    zlibstrm.avail_in = 0;
    zlibstrm.next_in = Z_NULL;
    int ret;
    ret = inflateInit2( &zlibstrm, windowbits );
    if ( ret!=Z_OK )
    {
	mErrRet( "Error Details:Initialization required to uncompress \
		data fails.\n", "Error type:", ret )
    }

    const od_uint32 chunksize = mMIN( mMaxChunkSize, srcfilesize_ );
    mAllocVarLenArr( unsigned char, in, chunksize );
    mAllocVarLenArr( unsigned char, out, chunksize );
    if ( !in.ptr() || !out.ptr()  )
    { mErrRet( "Unable to allot memory on the heap","","") }

    int count = chunksize;
    od_uint32 crc = 0;
    od_uint32 bytestowrite;
    int flushpolicy = Z_NO_FLUSH;
    do
    {
	if ( count <= srcfilesize_ ) 
	    isd_.istrm->read( mCast(char *,in.ptr()), chunksize );
	else
	{
	    isd_.istrm->read( mCast(char *,in.ptr()), srcfilesize_ % chunksize);
	    flushpolicy =  Z_FINISH ;
	}

	count += chunksize;
	zlibstrm.avail_in = isd_.istrm->gcount();
	if (zlibstrm.avail_in == 0 ) break;
	zlibstrm.next_in = in.ptr();
	do
	{
	    zlibstrm.avail_out = chunksize;
	    zlibstrm.next_out = out.ptr();
	    ret = inflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 )
	    {
		(void)inflateEnd( &zlibstrm );
		mErrRet( "Failed to unzip ", srcfile_, "\nZip file is corrupt" )
	    }

	    bytestowrite = chunksize - zlibstrm.avail_out;
	    crc = crc32( crc, mCast(Bytef*,out.ptr()), bytestowrite );
	    osd_.ostrm->write( mCast(char*,out.ptr()), bytestowrite );
	    if ( !*osd_.ostrm )
	    {
		(void)inflateEnd( &zlibstrm );
		mErrRet( "Failed to unzip ", srcfile_, "\nError occured while \
						    writing to disk" )
	    }

	} while ( zlibstrm.avail_out == 0  );

    } while ( flushpolicy != Z_FINISH );

    inflateEnd( &zlibstrm );
    if ( !(crc == crc_) )
    {
	mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt. " )
    }

    return ret == Z_STREAM_END ? true : false;
#else
    pErrMsg( "ZLib not available" );
    return false;
#endif
}


bool ZipHandler::getBitValue( const unsigned char byte, int bitposition ) const
{
    unsigned char modfbyte;
    modfbyte = byte >> ( bitposition );
    if ( modfbyte % 2 == 0)
	return false;

    return true;
}


void ZipHandler::setBitValue(unsigned char& byte,
					 int bitposition, bool value) const
{
    unsigned char var = mCast( unsigned char, pow(2.0 ,(bitposition)) );
    if ( value ) 
	byte = byte | var;
    else 
    {
	var = ~ var;
	byte = byte & var;
    }

}


const char* ZipHandler::errorMsg()const
{  return errormsg_.buf(); }


od_int16 ZipHandler::timeInDosFormat( const char* fnm )const
{
    unsigned char bte[2];
    QFileInfo qfi( fnm );
    QTime ftime = qfi.lastModified().time();
    int sec = ftime.second();
    char min = mCast( char, ftime.minute() );
    char hr = mCast( char, ftime.hour() );
    sec = sec/2;
    bte[0] = mCast( char, sec );
    bte[1] = 0;
    int idx;
    for ( idx=5; idx<8; idx++ )
	setBitValue( bte[0], idx, getBitValue(min,idx-5) );

    for ( idx=0; idx<3; idx++ )
	setBitValue( bte[1], idx, getBitValue(min,idx+3) );

    for ( idx=3; idx<8; idx++ )
	setBitValue( bte[1], idx, getBitValue(hr,idx-3) );

    od_uint16 dosformat;
    dosformat = *mCast( od_uint16*, bte );
    return dosformat;
}


od_int16 ZipHandler::dateInDosFormat( const char* fnm )const
{
    unsigned char bte[2];
    QFileInfo qfi( fnm );
    QDate fdate = qfi.lastModified().date();
    unsigned char day = mCast( char, fdate.day() );
    unsigned char month = mCast( char, fdate.month() );
    int year = fdate.year();
    unsigned char dosyear;
    dosyear = mCast( unsigned char, (year - 1980) );
    bte[0] =  day;
    bte[1] = 0;
    int idx;
    for ( idx = 5; idx < 8; idx++ )
	setBitValue( bte[0], idx, getBitValue(month,idx-5) );

    for ( idx = 0; idx < 1; idx++ )
	setBitValue( bte[1], idx, getBitValue(month,idx+3) );

    for ( idx = 1; idx < 8; idx++ )
	setBitValue( bte[1], idx, getBitValue(dosyear,idx-1) );

    od_uint16 dosformat;
    dosformat = *mCast( od_uint16*, bte );
    return dosformat;
}


bool ZipHandler::setTimeDateModified( const char* fnm, od_uint16 timeindos, 
				      od_uint16 dateindos ) const
{
    if ( timeindos == 0 || dateindos == 0 )
	return false;

    unsigned char bytetime[mSizeTwoBytes], bytedate[mSizeTwoBytes], byte = 0;
    int sec, min, hour, day, month, year, idx;
    bytetime[0] = *mCast( char*, &timeindos );
    bytetime[1] = *( mCast(char*,&timeindos)+1 );
    bytedate[0] = *mCast( char*, &dateindos );
    bytedate[1] = *( mCast(char*,&dateindos)+1 );
    for ( idx=0; idx<5; idx++ )
	setBitValue( byte, idx, getBitValue(bytetime[0],idx) );

    sec = byte * 2;
    byte = 0;
    for ( idx=0; idx<3; idx++ )
	setBitValue( byte, idx, getBitValue(bytetime[0],idx+5) );

    for ( idx=3; idx<6; idx++ )
	setBitValue( byte, idx, getBitValue(bytetime[1],idx-3) );

    min = byte;
    byte = 0;
    for ( idx=0; idx<5; idx++ )
	setBitValue( byte, idx, getBitValue(bytetime[1],idx+3) );

    hour = byte;
    byte = 0;
    for ( idx=0; idx<5; idx++ )
	setBitValue( byte, idx, getBitValue(bytedate[0],idx) );

    day = byte;
    byte = 0;
    for ( idx=0; idx<3; idx++ )
	setBitValue( byte, idx, getBitValue(bytedate[0],idx+5) );

    for ( idx=3; idx<4; idx++ )
	setBitValue( byte, idx, getBitValue(bytedate[1],idx-3) );

    month = byte;
    byte = 0;
    for ( idx=0; idx<7; idx++ )
	setBitValue( byte, idx, getBitValue(bytedate[1],idx+1) );

    year = byte + 1980;
    QTime qt( hour, min, sec );
    QDate qd( year, month, day );
    QDateTime qdt( qd, qt );
    od_int64 timeinsec;
    timeinsec = qdt.toTime_t();
#ifdef __win__
    struct _utimbuf ut;
    ut.modtime = timeinsec;
    ut.actime = timeinsec;
    if ( _utime( fnm, &ut) == -1 )
	return false;
#else
    struct utimbuf ut;
    ut.modtime = timeinsec;
    if ( utime( fnm, &ut) == -1 )
	return false;
#endif
    return true;
}


StreamData ZipHandler::makeOStreamForAppend( const char* fnm ) const
{
    StreamData sd;
    std::fstream* os = new std::fstream( fnm, std::ios::ios_base::in 
					    | std::ios::ios_base::out 
					    | std::ios::ios_base::binary);
    sd.ostrm = os;
    return sd;
}


int ZipHandler::getCumulativeFileCount( int dir ) const
{
    if ( cumulativefilecounts_.validIdx(dir) )
	return cumulativefilecounts_[dir];

    return -1;
}
