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
#include "ptrman.h"
#include "separstr.h"
#include "strmoper.h"
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


#define mLocalFileHeaderSig(T) \
    T[0] = 80; \
    T[1] = 75; \
    T[2] = 3; \
    T[3] = 4; \

#define mCntrlDirHeaderSig(T) \
    T[0] = 80; \
    T[1] = 75; \
    T[2] = 1; \
    T[3] = 2; \

#define mCntrlDirDigitalSig(T, ptr) \
    T[ptr + 0] = 80; \
    T[ptr + 1] = 75; \
    T[ptr + 2] = 5; \
    T[ptr + 3] = 5; \

#define mEndOfCntrlDirHeaderSig(T) \
    T[0] = 80; \
    T[1] = 75; \
    T[2] = 5; \
    T[3] = 6; \

#define mZIP64EndOfDirRecordHeaderSig(T) \
    T[0] = 80; \
    T[1] = 75; \
    T[2] = 6; \
    T[3] = 6; \

#define mZIP64EndOfDirLocatorHeaderSig(T) \
    T[0] = 80; \
    T[1] = 75; \
    T[2] = 6; \
    T[3] = 7; \

#define mInsertToCharBuff( inserttobuffer, datatype, insertat, len) \
    buf = ( char* ) &datatype;\
    for( int idx=0; idx<len; idx++ ) \
	inserttobuffer[insertat+idx] = *(buf + idx); 

#define mCntrlFileHeaderSigCheck( buff, ptr ) \
    sigcheck = false; \
    if( buff[ptr] == 80 && buff[ptr + 1] == 75 && \
	    buff[ptr + 2] == 1 && buff[ptr + 3] == 2 ) \
	    sigcheck = true; 

#define mFileHeaderSigCheck( buff, ptr ) \
    sigcheck = false; \
    if( buff[ptr] == 80 && buff[ptr + 1] == 75 && \
	    buff[ptr + 2] == 3 && buff[ptr + 3] == 4 ) \
	    sigcheck = true; \
    else \
	sigcheck = false; 

#define mHeaderSize 30
#define mCentralHeaderSize 46
#define mEndOfDirHeaderSize 22

#define mVerNeedToExtract 45
#define mDeflate 8
#define mNoCompression 0
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
#define mSizeEightBytes 8
#define mMaxChunkSize 10485760	    //10MB
#define m32BitSizeLimit 4294967294  //4GB
#define m16BitSizeLimit 65534
#define mSizeOfZIP64Header 20
#define mLZIP64HeaderID 0
#define mLZIP64DataSize 2
#define mLZIP64UnCompSize 4
#define mLZIP64CompSize 12
#define mLZIP64RelOffset 20
#define mZIP64EndOfDirRecordSize 56
#define mZIP64EndOfDirLocatorSize 20
#define mLZIP64EndOfDirRecordOffset 8
#define mLZIP64EndOfDirLocatorTotalDisks 16
#define mLZIP64CentralDirOffset 48
#define mLZIP64CentralDirTotalEntry 32
#define mZIP64Tag 1

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
{}


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
			      BufferStringSet& filenames )
{

    DirList dlist( src, DirList::DirsOnly, 0 );
    DirList flist( src, DirList::FilesOnly, 0 );

    for( int idx=0; idx<dlist.size(); idx++ )
    {
	filenames.add( dlist.fullPath(idx) );
	getFileList( dlist.fullPath(idx), filenames);
    }

    for( int idx=0; idx<flist.size(); idx++)
    {
	filenames.add( flist.fullPath(idx) );
	totalsize_ += File::getFileSize( flist.fullPath(idx) );
    }
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
    const od_int64 ptrlocation = StrmOper::tell( *osd_.ostrm );
    if ( !setLocalFileHeader( ) )
	return false;

    if ( uncompfilesize_ == 0 )
	return true;

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
    const od_uint32 chunksize = mMIN( mMaxChunkSize, uncompfilesize_ );
    od_uint32 upperbound = deflateBound( &zlibstrm, chunksize );
    if ( ret != Z_OK ) 
    {
	mErrRet( "Error Details:Initialization required to compress data \
		fails.\n", "Error type:", ret )
    }

    compfilesize_ = 0;
    crc_ = 0;

    int flushpolicy = Z_FINISH;

    ArrPtrMan<char> in, out;
    mTryAllocPtrMan( in, char [chunksize] );
    mTryAllocPtrMan( out, char [upperbound] );
    if ( !in || !out )
    { mErrRet("Cannot allocate memory.","System is out of memory","") }
    od_uint32 bytestowrite;
    do
    {
	isd_.istrm->read( in, chunksize );
	zlibstrm.avail_in = isd_.istrm->gcount();
	nrdonesize_ += zlibstrm.avail_in;
	flushpolicy = isd_.istrm->eof() ? Z_FINISH : Z_NO_FLUSH;
	crc_ = crc32( crc_, mCast(Bytef*,in.ptr()), isd_.istrm->gcount());
	zlibstrm.next_in = mCast( Bytef*, in.ptr() );
	do
	{
	    zlibstrm.avail_out = upperbound;
	    zlibstrm.next_out = mCast( Bytef*, out.ptr() );
	    ret = deflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 )
	    {
		mErrRet( "Failed to zip ", srcfile_, "\nFile may contain \
						     corrupt data" )
	    }

	    bytestowrite = upperbound - zlibstrm.avail_out;
	    compfilesize_ = compfilesize_ + bytestowrite;
	    if ( bytestowrite > 0 ) 
		osd_.ostrm->write( out, bytestowrite );

	    if ( !*osd_.ostrm )
	    {
		(void) deflateEnd ( &zlibstrm );
		mWriteErr( destfile_ )
	    }

	} while ( zlibstrm.avail_out == 0 );

    } while( flushpolicy != Z_FINISH );

    deflateEnd( &zlibstrm );
    StrmOper::seek( *osd_.ostrm, ptrlocation + mLCRC32 );
    osd_.ostrm->write( mCast(const char*,&crc_), sizeof(od_uint32) );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    if ( uncompfilesize_ > m32BitSizeLimit )
    {
	const od_uint32 fullvalue = m32BitSizeLimit + 1;
	osd_.ostrm->write( mCast(const char*,&fullvalue), sizeof(od_uint32));
	StrmOper::seek( *osd_.ostrm, ptrlocation + mHeaderSize + srcfnmsize_ + 
				     mLZIP64CompSize );
	osd_.ostrm->write( mCast(const char*,&compfilesize_), sizeof(od_int64));
	StrmOper::seek( *osd_.ostrm, ptrlocation + mHeaderSize + srcfnmsize_ + 
			mSizeOfZIP64Header + compfilesize_ );
    }
    else
    {
	osd_.ostrm->write( mCast(const char*,&compfilesize_),sizeof(od_uint32));
	StrmOper::seek( *osd_.ostrm, ptrlocation + mHeaderSize + srcfnmsize_ + 
				     compfilesize_ );
    }

    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
#else
    pErrMsg( "ZLib library not available" );
    return false;
#endif
}


bool ZipHandler::setLocalFileHeader()
{
    od_int64 srcfilesize;
    uncompfilesize_ = File::getFileSize(srcfile_);
    if ( uncompfilesize_ > m32BitSizeLimit )
	srcfilesize = m32BitSizeLimit + 1;
    else
	srcfilesize = uncompfilesize_;

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
    const od_uint32 nullvalue = 0;
    const od_uint16 version = mVerNeedToExtract;
    const od_uint16 compmethod = mDeflate;
    od_uint16 xtrafldlength = mSizeOfZIP64Header;

    mLocalFileHeaderSig ( headerbuff );
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtract, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLGenPurBitFlag, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, compmethod, mLCompMethod, mSizeTwoBytes );
    switch ( complevel_ )
    {
    case Maximum:
	setBitValue( headerbuff[mLGenPurBitFlag], 1, 1 );
    case Fast:
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
    case SuperFast:
	setBitValue( headerbuff[mLGenPurBitFlag], 1, 1 );
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
    case NoComp:
	mInsertToCharBuff( headerbuff, nullvalue, mLCompMethod, mSizeTwoBytes );
    default:
	setBitValue( headerbuff[mLGenPurBitFlag], 1, 0 );
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 0 );
    };

    const od_uint16 dostime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dostime, mLLastModFTime, mSizeTwoBytes );
    const od_uint16 dosdate = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dosdate, mLLastModFDate, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCRC32, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, srcfilesize, mLUnCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBytes );
    if ( uncompfilesize_ <= m32BitSizeLimit )
	xtrafldlength = 0;

    mInsertToCharBuff( headerbuff, xtrafldlength, mLExtraFldLength, 
		       mSizeTwoBytes );
    osd_.ostrm->write( mCast(const char*,headerbuff), mHeaderSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    osd_.ostrm->write( srcfnm.buf(), srcfnmsize_ );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    if ( uncompfilesize_ > m32BitSizeLimit )
	setZIP64Header();

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


bool ZipHandler::setZIP64Header()
{
    unsigned char headerbuff[mSizeOfZIP64Header];
    const od_uint16 headerid  = 1;
    const od_uint16 datasize = 16;
    char* buf = 0;
    mInsertToCharBuff( headerbuff, headerid, mLZIP64HeaderID, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, datasize, mLZIP64DataSize, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, uncompfilesize_, mLZIP64UnCompSize, 
		       mSizeEightBytes );
    osd_.ostrm->write( mCast(const char*,headerbuff), mSizeOfZIP64Header );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::setEndOfArchiveHeaders()
{
    const od_int64 ptrlctn = StrmOper::tell( *osd_.ostrm );
    bool ret = setCentralDirHeader();
    if ( !ret )
    {
	osd_.close();
	return false;
    }

    const od_int64 ptrlctn2 = StrmOper::tell( *osd_.ostrm );
    const od_uint32 sizecntrldir = ptrlctn2 - ptrlctn;
    const od_int64 totalentries = cumulativefilecounts_.last() +
							    initialfilecount_;
    if ( ptrlctn > m32BitSizeLimit || totalentries > m16BitSizeLimit )
    {
	if ( !setZIP64EndOfDirRecord(ptrlctn) )
	{
	    osd_.close();
	    return false;
	}

	if ( !setZIP64EndOfDirLocator(ptrlctn2) )
	{
	    osd_.close();
	    return false;
	}
    }

    ret = setEndOfCentralDirHeader ( ptrlctn, sizecntrldir );
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
	od_uint32 offset32bit;
	od_uint16 xtrafldlength;
	char localheader[1024];
	offsetoflocalheader_ = StrmOper::tell( *readdest.istrm );
	offset32bit = offsetoflocalheader_;
	readdest.istrm->read( mCast(char*,localheader), mHeaderSize);
	if ( !*readdest.istrm )
	{ mReadErr( destfile_ ) }

	localheader[mHeaderSize] = 0;
	compfilesize_ = *(od_uint32*)( localheader + mLCompSize );
	uncompfilesize_ = *(od_uint32*)( localheader + mLUnCompSize );
	srcfnmsize_ = *(od_uint16*)( localheader + mLFnmLength );
	xtrafldlength = *(od_uint16*)( localheader + mLExtraFldLength );
	readdest.istrm->read( mCast(char*,localheader+mHeaderSize),srcfnmsize_);
	localheader[mHeaderSize + srcfnmsize_] = 0;
	if ( offsetoflocalheader_ > m32BitSizeLimit || uncompfilesize_ > 
			    m32BitSizeLimit || compfilesize_ > m32BitSizeLimit )
	{
	    offset32bit = m32BitSizeLimit + 1;
	    unsigned char zip64headerbuff[100];
	    if ( xtrafldlength > 0 )
	    {
		readdest.istrm->read( mCast(char*,zip64headerbuff), 
				      xtrafldlength );
		mInsertToCharBuff( zip64headerbuff, offsetoflocalheader_, 
				   mLZIP64RelOffset, mSizeEightBytes );
		xtrafldlength += 8;
		for ( int id=0; id<xtrafldlength; id++ )
		    headerbuff[id+mCentralHeaderSize+srcfnmsize_] = 
							    zip64headerbuff[id];
		compfilesize_ = *(od_int64*)( zip64headerbuff + 
					      mLZIP64CompSize );
	    }
	    else
	    {
		const od_uint16 headerid  = 1;
		const od_uint16 datasize = 8;
		char* buf = 0;
		mInsertToCharBuff( zip64headerbuff, headerid, mLZIP64HeaderID,	
				   mSizeTwoBytes );
		mInsertToCharBuff( zip64headerbuff, datasize, mLZIP64DataSize, 
				   mSizeTwoBytes );
		mInsertToCharBuff( zip64headerbuff, offsetoflocalheader_, 4,
				   mSizeEightBytes );
		xtrafldlength = 12;
		for ( int id=0; id<xtrafldlength; id++ )
		    headerbuff[id+mCentralHeaderSize+srcfnmsize_] = 
							    zip64headerbuff[id];
	    }
	}

	for( int id=mSizeFourBytes; id<mHeaderSize; id++ )
	    headerbuff[id + 2] = localheader[id];

	for( int id=0; id<srcfnmsize_; id++ )
	    headerbuff[id + mCentralHeaderSize] = localheader[id + mHeaderSize];

	mInsertToCharBuff( headerbuff, offset32bit, mLRelOffset, 
			   mSizeFourBytes );
	mInsertToCharBuff( headerbuff, xtrafldlength, mLExtraFldLengthCentral, 
			   mSizeTwoBytes );
	StrmOper::seek( *readdest.istrm, StrmOper::tell(*readdest.istrm) +
					 compfilesize_ );
	osd_.ostrm->write( mCast(char*,headerbuff), 
                           mCentralHeaderSize + srcfnmsize_ + xtrafldlength );
	if ( !*osd_.ostrm )
	{ mWriteErr( destfile_ ) }

    } 

    readdest.close();
    return true;
}


bool ZipHandler::setZIP64EndOfDirRecord( od_int64 ptrlctn )
{
    char headerbuff[mZIP64EndOfDirRecordSize];
    char* buf;
    mZIP64EndOfDirRecordHeaderSig( headerbuff );

    const od_int64 sizeofheader = mZIP64EndOfDirRecordSize - 12;
    mInsertToCharBuff( headerbuff, sizeofheader, mLSizeOfData, mSizeEightBytes);

    const od_uint32 zipversion = 63;	    //Zip version used is 6.3
    mInsertToCharBuff( headerbuff, zipversion, mLVerMadeBy+8, mSizeTwoBytes );

    const od_uint16 version = mVerNeedToExtract;
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtract+10, 
		       mSizeTwoBytes );

    const od_uint32 nullvalue = 0;
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo+12, mSizeFourBytes);
    mInsertToCharBuff( headerbuff, nullvalue, mLCentralDirDiskNo+14, 
		       mSizeFourBytes);

    const od_int64 totalentries = cumulativefilecounts_.last() +
							    initialfilecount_;
    mInsertToCharBuff( headerbuff, totalentries, mLTotalEntryOnDisk+16,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, totalentries, mLTotalEntry+22,
		       mSizeEightBytes);

    od_int64 ptrlocation = StrmOper::tell( *osd_.ostrm );
    od_int64 sizecntrldir = ptrlocation - ptrlctn;
    mInsertToCharBuff( headerbuff, sizecntrldir, mLSizeCentralDir+28,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, ptrlctn, mLOffsetCentralDir+32,
		       mSizeEightBytes);

    osd_.ostrm->write( mCast(char*,headerbuff), mZIP64EndOfDirRecordSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}


bool ZipHandler::setZIP64EndOfDirLocator( od_int64 ptrlctn )
{
    char headerbuff[mZIP64EndOfDirLocatorSize];
    char* buf;
    const od_uint32 nullvalue = 0;
    const od_uint32 numberofdisks = 1;
    mZIP64EndOfDirLocatorHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeFourBytes);
    mInsertToCharBuff( headerbuff, ptrlctn, mLZIP64EndOfDirRecordOffset, 
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, numberofdisks, mLDiskNo+12, mSizeFourBytes);
    osd_.ostrm->write( mCast(char*,headerbuff), mZIP64EndOfDirLocatorSize );
    if ( !*osd_.ostrm )
    { mWriteErr( destfile_ ) }

    return true;
}



bool ZipHandler::setEndOfCentralDirHeader( od_int64 ptrlctn, 
					   od_uint32 sizecntrldir )
{
    char headerbuff[100];
    mEndOfCntrlDirHeaderSig( headerbuff );
    const od_uint32 nullvalue = 0;
    char* buf;
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCentralDirDiskNo,mSizeTwoBytes);
    if ( ptrlctn > m32BitSizeLimit )
	ptrlctn = m32BitSizeLimit + 1;

    cumulativefilecounts_[(cumulativefilecounts_.size()-1)] = 
			  cumulativefilecounts_.last() + initialfilecount_;
    od_uint16 cumulativefilecount = cumulativefilecounts_.last();
    if ( cumulativefilecounts_.last() > m16BitSizeLimit )
	cumulativefilecount = m16BitSizeLimit + 1;

    mInsertToCharBuff( headerbuff, cumulativefilecount, mLTotalEntryOnDisk, 
		       mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, cumulativefilecount, mLTotalEntry, 
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

    StrmOper::seek( *osd_.ostrm, offsetofcentraldir_ );
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

    if ( !readCentralDirHeader(&zfileinfo) )
    {
	isd_.close();
	return false;
    }

    isd_.close();
    return true;
}


bool ZipHandler::initUnZipArchive( const char* srcfnm, const char* basepath )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

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

    if ( !readCentralDirHeader() )
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

    StrmOper::seek( *isd_.istrm, offset );
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


bool ZipHandler::readCentralDirHeader( ObjectSet<ZipFileInfo>* zfileinfo )
{
    if ( offsetofcentraldir_ == 0 )
    {
	if ( !readEndOfCentralDirHeader() )
	    return false;
    }
    
    StrmOper::seek( *isd_.istrm, offsetofcentraldir_ );
    od_int64 ptrlocation;
    ptrlocation = StrmOper::tell( *isd_.istrm );
    char headerbuff[1024];
    bool sigcheck;
    for ( int idx=0; idx<cumulativefilecounts_.last(); idx++ )
    {
	isd_.istrm->read( mCast(char*,headerbuff), mCentralHeaderSize );
	headerbuff[mCentralHeaderSize] = '\0';
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    isd_.close();
	    mErrRet( srcfile_, " is not a valid zip archive", "" )
	}

	if ( getBitValue( *(headerbuff + mLCentralDirBitFlag), 0 ) )
	{
	    mErrRet( "Encrypted file::Not supported", "", "" )
	    ////TODO
	}

	od_uint16 version = *mCast( od_uint16*, headerbuff + 
							mLCentralDirVersion );
	if ( version > mVerNeedToExtract )
	{ mErrRet("Failed to unzip ", srcfile_, 
		   "\nVersion of zip format needed to unpack is not supported")}

	od_uint16 compmethod = *mCast( od_uint16*, headerbuff + 
						    mLCentralDirCompMethod );
	if ( compmethod != mDeflate && compmethod != 0 )
	{ mErrRet( "Failed to unzip ", srcfile_, "\nCompression method used \
						   is not supported" ) }

	od_uint16 bitflag = *mCast( od_uint16*, headerbuff + 
						    mLCentralDirBitFlag );
	if ( bitflag > 14 )
	{ mErrRet("Failed to unzip ", srcfile_, 
		   "\nVersion of zip format needed to unpack is not supported")}

	StrmOper::seek( *isd_.istrm, ptrlocation + mCentralHeaderSize );
	BufferString headerfnm;
	isd_.istrm->read( headerfnm.buf(),
			  *mCast(od_uint16*,headerbuff+mLFnmLengthCentral) );
	headerfnm[*((od_uint16*)(headerbuff+mLFnmLengthCentral))] = '\0';
	od_uint16 xtrafldlength = *mCast( od_uint16*,
                                          headerbuff + mLExtraFldLengthCentral);
	if ( xtrafldlength > 0 )
	    isd_.istrm->read( headerbuff + mCentralHeaderSize, xtrafldlength );
	    
	uncompfilesize_ = *(od_uint32*)(headerbuff+mLUnCompSizeCentral);
	compfilesize_ = *(od_uint32*)(headerbuff+mLCompSizeCentral);
	offsetoflocalheader_ = *(od_uint32*)(headerbuff+mLRelOffset);
	if ( uncompfilesize_ > m32BitSizeLimit || 
             compfilesize_ > m32BitSizeLimit || 
             offsetoflocalheader_ > m32BitSizeLimit )
	    readXtraFldForZIP64(headerbuff + mCentralHeaderSize, xtrafldlength);

	if ( zfileinfo )
	    *zfileinfo += new ZipFileInfo( srcfile_, compfilesize_,
					   uncompfilesize_, 
offsetoflocalheader_ );

	totalsize_ += uncompfilesize_;
	ptrlocation = ptrlocation
			+ *mCast( od_uint16*,headerbuff+mLFnmLengthCentral )
			+ *mCast( od_uint16*,headerbuff+mLExtraFldLengthCentral)
			+ *mCast( od_uint16*,headerbuff+mLFileComntLength )
			+ mCentralHeaderSize;
	StrmOper::seek( *isd_.istrm, ptrlocation );
    }

    StrmOper::seek( *isd_.istrm, 0 );
    return true;
}


bool ZipHandler::readEndOfCentralDirHeader()
{
    char headerbuff[mEndOfDirHeaderSize];
    od_int64 ptrlocation;
    char sig[mSizeFourBytes];
    mEndOfCntrlDirHeaderSig( sig );
    StrmOper::seek( *isd_.istrm, 0, std::ios::end );
    ptrlocation = StrmOper::tell( *isd_.istrm );
    if ( ptrlocation == 0 )
    { mErrRet( "Zip archive is empty", "", "" ) }

    StrmOper::seek( *isd_.istrm, ptrlocation - mEndOfDirHeaderSize );
    ptrlocation = StrmOper::tell( *isd_.istrm );
    isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    while (!( *mCast(od_uint32*,headerbuff) == *mCast(od_uint32*,sig) ))
    {
	StrmOper::seek( *isd_.istrm, ptrlocation - 1 );
	ptrlocation = StrmOper::tell( *isd_.istrm );
	if ( ptrlocation == 0 || ptrlocation == -1 )
	{ mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

	isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    }

    isd_.istrm->read( mCast(char*,headerbuff+mSizeFourBytes),
				        mEndOfDirHeaderSize-mSizeFourBytes );
    StrmOper::seek( *isd_.istrm, 0 );
    offsetofcentraldir_ = *mCast( od_uint32*, headerbuff+mLOffsetCentralDir );
    od_uint16 cumulativefilecount = *mCast(od_uint16*, headerbuff+mLTotalEntry);
    if ( offsetofcentraldir_ > m32BitSizeLimit || cumulativefilecount > 
							     m16BitSizeLimit )
    { return readZIP64EndOfCentralDirLocator(); }

    cumulativefilecounts_ += cumulativefilecount;
    return true;
}


bool ZipHandler::readZIP64EndOfCentralDirLocator()
{
    char headerbuff[mZIP64EndOfDirLocatorSize];
    od_int64 ptrlocation;
    char sig[mSizeFourBytes];
    mZIP64EndOfDirLocatorHeaderSig( sig );
    StrmOper::seek( *isd_.istrm, 0, std::ios::end );
    ptrlocation = StrmOper::tell( *isd_.istrm );
    StrmOper::seek( *isd_.istrm, ptrlocation - mEndOfDirHeaderSize - 
					       mZIP64EndOfDirLocatorSize );
    ptrlocation = StrmOper::tell( *isd_.istrm );
    isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    while (!( *mCast(od_uint32*,headerbuff) == *mCast(od_uint32*,sig) ))
    {
	StrmOper::seek( *isd_.istrm, ptrlocation - 1 );
	ptrlocation = StrmOper::tell( *isd_.istrm );
	if ( ptrlocation == 0 || ptrlocation == -1 )
	{ mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

	isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    }

    isd_.istrm->read( mCast(char*,headerbuff+mSizeFourBytes),
		      mZIP64EndOfDirLocatorSize-mSizeFourBytes );
    offsetofcentraldir_ = *mCast( od_int64*, 
                                  headerbuff + mLZIP64EndOfDirRecordOffset );
    int totaldisks = *mCast( int*, headerbuff+mLZIP64EndOfDirLocatorTotalDisks);
    if ( totaldisks > 1 )
	{ mErrRet( "Failed to unzip ", srcfile_, 
		"\nMultiple disk spanning of zip archive is not supported" ) }
	
    return readZIP64EndOfCentralDirRecord();
}


bool ZipHandler::readZIP64EndOfCentralDirRecord()
{
    char headerbuff[mZIP64EndOfDirRecordSize];
    od_int64 ptrlocation;
    char sig[mSizeFourBytes];
    mZIP64EndOfDirRecordHeaderSig( sig );
    StrmOper::seek( *isd_.istrm, offsetofcentraldir_ );
    ptrlocation = StrmOper::tell( *isd_.istrm );
    isd_.istrm->read( mCast(char*,headerbuff), mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    if ( *mCast(od_uint32*,headerbuff) != *mCast(od_uint32*,sig) )
    { mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

    isd_.istrm->read( mCast(char*,headerbuff+mSizeFourBytes),
				     mZIP64EndOfDirRecordSize-mSizeFourBytes );
    offsetofcentraldir_ = *mCast( od_int64*, headerbuff +
					     mLZIP64CentralDirOffset );
    cumulativefilecounts_ += *mCast( od_int64*, headerbuff + 
						mLZIP64CentralDirTotalEntry );
    StrmOper::seek( *isd_.istrm, 0 );
    return true;
}


bool ZipHandler::readXtraFldForZIP64( const char* xtrafld, int size )
{
    int idx = 0;
    while ( true )
    {
	if ( idx >= size )
	    break;

	if ( *mCast(od_uint16*,xtrafld + idx) != mZIP64Tag )
	{
	    const od_uint16 sizeofblock = *mCast(od_uint16*, xtrafld + idx + 2);
	    idx += (sizeofblock+mSizeFourBytes);
	    continue;
	}
	else
	{
	    od_uint16 sizeofblock = *mCast(od_uint16*, xtrafld + idx + 2);
	    if ( uncompfilesize_ == m32BitSizeLimit + 1 && sizeofblock > 4 )
	    {
		uncompfilesize_ = *mCast( od_int64*, xtrafld + idx + 
						     mSizeFourBytes );
		idx += mSizeEightBytes;
		sizeofblock -= mSizeEightBytes;
	    }

	    if ( compfilesize_ == m32BitSizeLimit + 1 && sizeofblock > 4 )
	    {
		compfilesize_ = *mCast( od_int64*, xtrafld+idx+mSizeFourBytes );
		idx += mSizeEightBytes;
		sizeofblock -= mSizeEightBytes;
	    }

	    if ( offsetoflocalheader_ == m32BitSizeLimit + 1 && sizeofblock > 4)
	    {
		offsetoflocalheader_ = *mCast( od_int64*, 
					       xtrafld+idx+mSizeFourBytes );
		idx += mSizeEightBytes;
	    }

	    break;
	}
    }

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

    if ( compfilesize_ == 0 )
    {}
    else if ( compmethod_ == mDeflate )
    {
	if ( !doZUnCompress() )
	{
	    osd_.close();
	    isd_.close();
	    return false;
	}
    }
    else if ( compmethod_ == mNoCompression )
    {
	const od_uint32 chunksize = mMIN( mMaxChunkSize, compfilesize_ );
	od_int64 count = chunksize;
	bool finish = false;
	ArrPtrMan<char> in;
	mTryAllocPtrMan( in, char [chunksize] );
	if ( !in )
	{ mErrRet("Cannot allocate memory.","System is out of memory","") }
	do
	{
	    if ( count <= compfilesize_ )
	    {
		isd_.istrm->read( in, chunksize );
		osd_.ostrm->write( in, chunksize );
	    }
	    else
	    {
		isd_.istrm->read( in, compfilesize_ % chunksize );
		osd_.ostrm->write ( in, compfilesize_ % chunksize);
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
    const od_int64 ptrlocation = StrmOper::tell( *isd_.istrm );
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
    compfilesize_ = *mCast( od_uint32*, headerbuff + mLCompSize );
    uncompfilesize_ = *mCast( od_uint32*, headerbuff + mLUnCompSize );
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

	StrmOper::seek( *isd_.istrm, ptrlocation + mHeaderSize + srcfnmsize_ + 
				     xtrafldlth );
	return 2;
    }

    destfile_ = destbasepath_;
    destfile_ += headerbuff;
    if ( xtrafldlth > 0 && compfilesize_ > m32BitSizeLimit )
    {
	isd_.istrm->read( mCast(char*,headerbuff), xtrafldlth );
	readXtraFldForZIP64( headerbuff, xtrafldlth );
    }

    StrmOper::seek( *isd_.istrm, ptrlocation + mHeaderSize + srcfnmsize_ + 
				 xtrafldlth );
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

    const od_uint32 chunksize = mMIN( mMaxChunkSize, compfilesize_ );
    ArrPtrMan<char> in, out;
    mTryAllocPtrMan( in, char [chunksize] );
    mTryAllocPtrMan( out, char [chunksize] );
    if ( !in || !out )
    { mErrRet("Cannot allocate memory.","System is out of memory","") }
    od_int64 count = chunksize;
    od_uint32 crc = 0;
    od_uint32 bytestowrite;
    int flushpolicy = Z_NO_FLUSH;
    do
    {
	if ( count <= compfilesize_ )
	{
	    od_int64 ptr1 = StrmOper::tell( *isd_.istrm );
	    isd_.istrm->read( in, chunksize );
	    ptr1 = StrmOper::tell( *isd_.istrm );
	}
	else
	{
	    isd_.istrm->read( in, compfilesize_ % chunksize);
	    flushpolicy =  Z_FINISH ;
	}

	count += chunksize;
	zlibstrm.avail_in = isd_.istrm->gcount();
	if (zlibstrm.avail_in == 0 ) break;
	zlibstrm.next_in = mCast( Bytef*, in.ptr() );
	do
	{
	    zlibstrm.avail_out = chunksize;
	    zlibstrm.next_out = mCast( Bytef*, out.ptr() );
	    ret = inflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 && ret != Z_BUF_ERROR )
	    {
		(void)inflateEnd( &zlibstrm );
		mErrRet( "Failed to unzip ", srcfile_, "\nZip file is corrupt" )
	    }

	    bytestowrite = chunksize - zlibstrm.avail_out;
	    nrdonesize_ += bytestowrite;
	    crc = crc32( crc, mCast(Bytef*,out.ptr()), bytestowrite );
	    osd_.ostrm->write( out, bytestowrite );
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


od_uint16 ZipHandler::timeInDosFormat( const char* fnm )const
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


od_uint16 ZipHandler::dateInDosFormat( const char* fnm )const
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
    bytetime[1] = *( mCast(char*,&timeindos) + 1 );
    bytedate[0] = *mCast( char*, &dateindos );
    bytedate[1] = *( mCast(char*,&dateindos) + 1 );
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
