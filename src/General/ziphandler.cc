/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Salil Agarwal
Date:          30 August 2012
________________________________________________________________________

-*/


#include "ziphandler.h"

#include "ziparchiveinfo.h"
#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "executor.h"
#include "ptrman.h"
#include "separstr.h"
#include "task.h"
#include "varlenarray.h"
#include "od_iostream.h"

#ifndef OD_NO_QT
#include <QFileInfo>
#include <QDate>
#endif

#ifdef HAS_ZLIB
#include "zlib.h"
#endif

#ifdef __win__
#include "sys/utime.h"
#include "Windows.h"
#else
#include "sys/stat.h"
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

#define mLDOSFileAttr 0
#define mLUNIXFileAttr 2
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
#define mLOSMadeBy 5
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
#define mSizeOneBytes 1
#define mSizeTwoBytes 2
#define mSizeThreeBytes 3
#define mSizeFourBytes 4
#define mSizeEightBytes 8
#define mMaxChunkSize 10485760	    //10MB
#define m32BitSizeLimit 4294967294UL  //4GB
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
#define mIsError 0
#define mIsFile 1
#define mIsDirectory 2
#define mIsLink 3
#define mZDefaultMemoryLevel 9
#define mMaxWindowBitForRawDeflate -15

#define mErrRet(pre,mid,post) { \
    errormsg_.set( pre ).add( mid ).add( post ); \
    return false; }


union FileAttr
{
    od_uint32 integer_;
    char      bytes_[4];
};


ZipHandler::~ZipHandler()
{
    if ( istrm_ )
	{ pErrMsg( "istrm_ still open" ); closeInputStream(); }
    if ( ostrm_ )
	{ pErrMsg( "ostrm_ still open" ); closeOutputStream(); }
}


void ZipHandler::closeInputStream()
{
    delete istrm_; istrm_ = 0;
}


void ZipHandler::closeOutputStream()
{
    delete ostrm_; ostrm_ = 0;
}


bool ZipHandler::reportWriteError( const char* filenm ) const
{
    if ( !filenm ) filenm = destfile_.buf();

    errormsg_.set( "Unable to write to " ).add( filenm );
    if ( ostrm_ )
	ostrm_->addErrMsgTo( errormsg_ );

    return false;
}


bool ZipHandler::reportReadError( const char* filenm ) const
{
    return reportStrmReadError( istrm_, filenm );
}


bool ZipHandler::reportStrmReadError( od_istream* strm,
					const char* filenm ) const
{
    if ( !filenm ) filenm = srcfile_.buf();

    errormsg_.set( "Unable to read from " ).add( filenm );
    if ( strm )
	strm->addErrMsgTo( errormsg_ );

    return false;
}


bool ZipHandler::initMakeZip( const char* destfnm,
			      const BufferStringSet& srcfnms )
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
    ostrm_ = new od_ostream( destfile_ );
    return ostrm_->isOK() ? true : reportWriteError();
}


bool ZipHandler::getFileList( const char* src,
			      BufferStringSet& filenames )
{
    File::makeRecursiveFileList( src, filenames, false );
    for( int idx=0; idx<filenames.size(); idx++ )
	totalsize_ += File::getFileSize( filenames.get(idx) );

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
    switch( ret )
    {
    case mIsError:
        closeOutputStream();
	return false;
    case mIsFile:
        ret = doZCompress();
        closeInputStream();
        break;
    case mIsDirectory:
        ret = setLocalFileHeaderForDir();
        break;
    case mIsLink:
        ret = setLocalFileHeaderForLink();
        break;
    default:
        break;
    }

    if ( !ret )
	{ closeOutputStream(); return false; }

    curfileidx_++;
    return true;
}


int ZipHandler::openStrmToRead( const char* src )
{
    srcfile_ = src;
    if ( File::isLink(src) )
        return mIsLink;

    if ( File::isDirectory(src) )
	return mIsDirectory;

    if( !File::exists(src) )
    {
	errormsg_ = src;
	errormsg_ += " does not exist.";
	return mIsError;
    }

    istrm_ = new od_istream( src );
    if ( istrm_->isBad() )
    {
	errormsg_.set( "Unable to open " ).add( src );
	istrm_->addErrMsgTo( errormsg_ );
	return mIsError;
    }

    return mIsFile;
}


void ZipHandler::setCompLevel( CompLevel cl )
{ complevel_ = cl; }


bool ZipHandler::doZCompress()
{
#ifdef HAS_ZLIB
    const od_stream::Pos initialpos = ostrm_->position();
    if ( !setLocalFileHeader( ) )
	return false;
    else if ( uncompfilesize_ == 0 )
	return true;

    z_stream zlibstrm;
    zlibstrm.zalloc = Z_NULL; zlibstrm.zfree = Z_NULL; zlibstrm.opaque = Z_NULL;
    const int method = Z_DEFLATED;
    int ret = deflateInit2( &zlibstrm, complevel_, method,
	mMaxWindowBitForRawDeflate, mZDefaultMemoryLevel, Z_DEFAULT_STRATEGY );

    const od_uint32 chunksize = mMaxLimited( uncompfilesize_, mMaxChunkSize );
    od_uint32 upperbound = deflateBound( &zlibstrm, chunksize );
    if ( ret != Z_OK )
	mErrRet( "Error Details:Initialization required to compress data \
		fails.\n", "Error type:", ret )

    mAllocLargeVarLenArr( char, in, chunksize );
    mAllocLargeVarLenArr( char, out, upperbound );
    if ( !in || !out )
	mErrRet("Cannot allocate memory.","System is out of memory","")

    compfilesize_ = crc_ = 0;
    for ( int flushpolicy=Z_NO_FLUSH; flushpolicy!=Z_FINISH;  )
    {
	istrm_->getBin( in, chunksize );
	uInt nrbytesread = (uInt)istrm_->lastNrBytesRead();
	zlibstrm.avail_in = nrbytesread;
	nrdonesize_ += zlibstrm.avail_in;
	flushpolicy = istrm_->isOK() ? Z_NO_FLUSH : Z_FINISH;
	crc_ = crc32( crc_, mCast(Bytef*,in.ptr()), nrbytesread);
	zlibstrm.next_in = mCast( Bytef*, in.ptr() );

	zlibstrm.avail_out = 0;
	while ( zlibstrm.avail_out == 0 )
	{
	    zlibstrm.avail_out = upperbound;
	    zlibstrm.next_out = mCast( Bytef*, out.ptr() );
	    ret = deflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 )
		mErrRet( "Failed to zip ", srcfile_,
			 "\nFile may contain corrupt data" )

	    od_uint32 bytestowrite = upperbound - zlibstrm.avail_out;
	    compfilesize_ = compfilesize_ + bytestowrite;
	    if ( bytestowrite > 0 )
	    {
		ostrm_->addBin( out, bytestowrite );
		if ( !ostrm_->isOK() )
		    { (void)deflateEnd(&zlibstrm); return reportWriteError(); }
	    }

	}
    }

    deflateEnd( &zlibstrm );
    ostrm_->setWritePosition( initialpos + mLCRC32 );
    ostrm_->addBin( &crc_, sizeof(od_uint32) );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    if ( uncompfilesize_ > m32BitSizeLimit )
    {
	const od_uint32 fullvalue = m32BitSizeLimit + 1;
	ostrm_->addBin( &fullvalue, sizeof(od_uint32) );
	ostrm_->setWritePosition( initialpos + mHeaderSize + srcfnmsize_
			   + mLZIP64CompSize );
	ostrm_->addBin( &compfilesize_, sizeof(od_int64) );
	ostrm_->setWritePosition( initialpos + mHeaderSize + srcfnmsize_
			   + mSizeOfZIP64Header + compfilesize_ );
    }
    else
    {
	ostrm_->addBin( &compfilesize_, sizeof(od_uint32) );
	ostrm_->setWritePosition( initialpos + mHeaderSize + srcfnmsize_
			   + compfilesize_ );
    }

    if ( !ostrm_->isOK() )
	return reportWriteError();

    return true;
#else
    pErrMsg( "ZLib library not available" );
    return false;
#endif
}


bool ZipHandler::setLocalFileHeader()
{
    od_int64 srcfilesize;
    uncompfilesize_ = File::getFileSize(srcfile_,false);
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
    ostrm_->addBin( headerbuff, mHeaderSize );
    ostrm_->addBin( srcfnm.buf(), srcfnmsize_ );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    if ( uncompfilesize_ > m32BitSizeLimit )
	setZIP64Header();

    ostrm_->flush();
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

    ostrm_->addBin( headerbuff, mHeaderSize );
    ostrm_->addBin( srcfnm.buf(), srcfnmsize_ );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    ostrm_->flush();
    return true;
}


bool ZipHandler::setLocalFileHeaderForLink()
{
#ifdef HAS_ZLIB
    FilePath fnm( srcfile_ );
    int p = fnm.nrLevels();
    BufferString srcfnm = "";
    for ( int idx=(curnrlevels_-1); idx<=(p-2); idx++ )
    {
	srcfnm.add( fnm.dir( idx ) );
	srcfnm += "/";
    }

    srcfnm.add( fnm.fileName() );
    srcfnmsize_ = ( od_uint16 ) srcfnm.size();
    unsigned char headerbuff[1024];
    mLocalFileHeaderSig ( headerbuff );
    headerbuff[mLVerNeedToExtract] = mVerNeedToExtract;
    for ( int idx=5; idx<10; idx++ )
	headerbuff[idx] = '\0';

#ifdef __win__
    HANDLE filehandle = CreateFile ( srcfile_, GENERIC_READ, 0, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    od_int32 linksize = GetFileSize( filehandle, NULL );
    od_int32 bytesread;
    BufferString linkvalue( linksize, true );
    ReadFile( filehandle, linkvalue.getCStr(), linksize,
	    (LPDWORD)&bytesread, NULL);
    CloseHandle( filehandle );
#else
    BufferString linkvalue = File::linkValue( srcfile_ );
    od_uint32 linksize = linkvalue.size();
#endif

    od_uint32 crc = 0;
    crc = crc32(crc, mCast(Bytef*,linkvalue.buf()), linksize);
    char* buf = 0;
    const od_uint16 nullvalue = 0;
    const od_uint16 dostime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dostime, mLLastModFTime, mSizeTwoBytes );
    const od_uint16 dosdate= dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, dosdate, mLLastModFDate, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, crc, mLCRC32, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, linksize, mLCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, linksize, mLUnCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtraFldLength, mSizeTwoBytes );

    ostrm_->addBin( headerbuff, mHeaderSize );
    ostrm_->addBin( srcfnm.buf(), srcfnmsize_ );
    ostrm_->addBin( linkvalue.buf(), linksize );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    ostrm_->flush();
    return true;
#else
    pErrMsg( "ZLib not available" );
    return false;
#endif
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
    ostrm_->addBin( headerbuff, mSizeOfZIP64Header );
    return ostrm_->isOK() ? true : reportWriteError();
}


bool ZipHandler::setEndOfArchiveHeaders()
{
    const od_int64 startchpos = ostrm_->position();
    bool ret = setCentralDirHeader();
    if ( !ret )
	{ closeOutputStream(); return false; }

    const od_int64 endchpos = ostrm_->position();
    const od_uint32 sizecntrldir = endchpos - startchpos;
    const od_int64 totalentries = cumulativefilecounts_.last() +
							    initialfilecount_;
    if ( startchpos > m32BitSizeLimit || totalentries > m16BitSizeLimit )
    {
	if ( !setZIP64EndOfDirRecord(startchpos)
	  || !setZIP64EndOfDirLocator(endchpos) )
	    { closeOutputStream(); return false; }
    }

    ret = setEndOfCentralDirHeader ( startchpos, sizecntrldir );
    closeOutputStream();
    return ret;
}


bool ZipHandler::setCentralDirHeader()
{
    od_istream deststrm( destfile_ );
    if ( !deststrm.isOK() )
	return reportStrmReadError( &deststrm, destfile_ );

    char headerbuff[1024];
    char* buf;
    const od_uint32 nullvalue = 0;
    const od_uint32 zipversion = 63;	    //Zip version used is 6.3
    od_uint16 os;
#ifdef __win__
    os = 0;                                 // 0 for windows
#else
    os = 3;                                 // 3 for UNIX
#endif

    mCntrlDirHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, zipversion, mLVerMadeBy, mSizeOneBytes );
    mInsertToCharBuff( headerbuff, os, mLOSMadeBy, mSizeOneBytes );
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
	offsetoflocalheader_ = deststrm.position();
	offset32bit = offsetoflocalheader_;
	deststrm.getBin( localheader, mHeaderSize );
	if ( !deststrm.isOK() )
	    return reportStrmReadError( &deststrm, destfile_ );

	localheader[mHeaderSize] = 0;
	compfilesize_ = *(od_uint32*)( localheader + mLCompSize );
	uncompfilesize_ = *(od_uint32*)( localheader + mLUnCompSize );
	srcfnmsize_ = *(od_uint16*)( localheader + mLFnmLength );
	xtrafldlength = *(od_uint16*)( localheader + mLExtraFldLength );
	deststrm.getBin( localheader+mHeaderSize, srcfnmsize_ );
	localheader[mHeaderSize + srcfnmsize_] = 0;
        od_uint32 extattr = 0;
        if (  index >= initialfilecount_ )
            extattr = setExtFileAttr( index );

	if ( offsetoflocalheader_ > m32BitSizeLimit || uncompfilesize_ >
			    m32BitSizeLimit || compfilesize_ > m32BitSizeLimit )
	{
	    offset32bit = m32BitSizeLimit + 1;
	    unsigned char zip64headerbuff[100];
	    if ( xtrafldlength > 0 )
	    {
		deststrm.getBin( zip64headerbuff, xtrafldlength );
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
	mInsertToCharBuff( headerbuff, extattr, mLExtFileAttr, mSizeFourBytes );
	deststrm.ignore( compfilesize_ );
	ostrm_->addBin( headerbuff,
                        mCentralHeaderSize + srcfnmsize_ + xtrafldlength );
	if ( !ostrm_->isOK() )
	    return reportWriteError();

    }

    return true;
}


bool ZipHandler::setZIP64EndOfDirRecord( od_int64 eodpos )
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

    od_stream::Pos ptrlocation = ostrm_->position();
    od_int64 sizecntrldir = ptrlocation - eodpos;
    mInsertToCharBuff( headerbuff, sizecntrldir, mLSizeCentralDir+28,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, eodpos, mLOffsetCentralDir+32,
		       mSizeEightBytes);

    ostrm_->addBin( headerbuff, mZIP64EndOfDirRecordSize );
    return ostrm_->isOK() ? true : reportWriteError();
}


bool ZipHandler::setZIP64EndOfDirLocator( od_int64 eodpos )
{
    char headerbuff[mZIP64EndOfDirLocatorSize];
    char* buf;
    const od_uint32 nullvalue = 0;
    const od_uint32 numberofdisks = 1;
    mZIP64EndOfDirLocatorHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeFourBytes);
    mInsertToCharBuff( headerbuff, eodpos, mLZIP64EndOfDirRecordOffset,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, numberofdisks, mLDiskNo+12, mSizeFourBytes);

    ostrm_->addBin( headerbuff, mZIP64EndOfDirLocatorSize );
    return ostrm_->isOK() ? true : reportWriteError();
}



bool ZipHandler::setEndOfCentralDirHeader( od_int64 cdirpos,
					   od_uint32 sizecntrldir )
{
    char headerbuff[100];
    mEndOfCntrlDirHeaderSig( headerbuff );
    const od_uint32 nullvalue = 0;
    char* buf;
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCentralDirDiskNo,mSizeTwoBytes);
    if ( cdirpos > m32BitSizeLimit )
	cdirpos = m32BitSizeLimit + 1;

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
    mInsertToCharBuff( headerbuff, cdirpos, mLOffsetCentralDir,
		       mSizeFourBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLZipFileComntLength,
		       mSizeTwoBytes );

    ostrm_->addBin( headerbuff, mEndOfDirHeaderSize );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    ostrm_->flush();
    return true;
}


od_uint32 ZipHandler::setExtFileAttr( const od_uint32 index )
{
    char headerbuff[4];
    char* buf;
    union FileAttr fileattr;
#ifdef __win__
    fileattr.integer_ = GetFileAttributes ( allfilenames_.get(
                                                     index-initialfilecount_) );
    fileattr.bytes_[mLDOSFileAttr+1] = '\0';
    fileattr.bytes_[mLUNIXFileAttr] = '\0';
    fileattr.bytes_[mLUNIXFileAttr+1] = '\0';
    return fileattr.integer_;
#else
    struct stat filestat;
    int ret = lstat( allfilenames_.get(index-initialfilecount_), &filestat );
    if ( ret<0 )
        return 0;

    mInsertToCharBuff( headerbuff, filestat.st_mode, 0, mSizeTwoBytes );
    fileattr.bytes_[mLDOSFileAttr] = '\0';
    fileattr.bytes_[mLDOSFileAttr+1] = '\0';
    fileattr.bytes_[mLUNIXFileAttr] = headerbuff[0];
    fileattr.bytes_[mLUNIXFileAttr+1] = headerbuff[1];
    return fileattr.integer_;
#endif
}


bool ZipHandler::initAppend( const char* srcfnm, const char* fnm )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    FilePath fp( fnm );
    curnrlevels_ = fp.nrLevels();
    destfile_ = srcfnm;
    istrm_ = new od_istream( srcfnm );
    if ( istrm_->isBad() )
	return reportReadError( srcfnm );

    if ( !readEndOfCentralDirHeader() )
	return false;

    initialfilecount_ = cumulativefilecounts_.last();
    closeInputStream();
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

    ostrm_ = new od_ostream( srcfnm, true ); // open for edit
    if ( !ostrm_->isOK() )
	return reportWriteError( srcfnm );

    ostrm_->setWritePosition( offsetofcentraldir_ );
    return true;
}


bool ZipHandler::getArchiveInfo( const char* srcfnm,
				 ObjectSet<ZipFileInfo>& zfileinfo )
{
    if ( !File::exists(srcfnm) )
    { mErrRet( srcfnm, " does not exist", "" ) }

    istrm_ = new od_istream( srcfnm );
    if ( istrm_->isBad() )
	return reportReadError( srcfnm );

    if ( !readEndOfCentralDirHeader() )
	return false;

    const bool ret = readCentralDirHeader( &zfileinfo );
    closeInputStream();
    return ret;
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
    istrm_ = new od_istream( srcfnm );
    if ( istrm_->isBad() )
	return reportReadError( srcfnm );

    if ( !readEndOfCentralDirHeader() || !readCentralDirHeader() )
	{ closeInputStream(); return false; }

    return true;
}


bool ZipHandler::unZipFile( const char* srcfnm, const char* fnm,
			    const char* path )
{
    if ( !File::exists(srcfnm) )
	{ mErrRet( srcfnm, " does not exist", "" ) }

    ZipArchiveInfo zai( srcfnm );
    od_stream::Pos offset = zai.getLocalHeaderOffset( fnm );
    if ( offset == -1 )
	{ errormsg_ = zai.errorMsg(); return false; }

    istrm_ = new od_istream( srcfnm );
    if ( istrm_->isBad() )
	return reportReadError( srcfnm );

    if ( !readEndOfCentralDirHeader() )
	{ closeInputStream(); return false; }

    istrm_->setReadPosition( offset );
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

    closeInputStream();
    return true;
}


bool ZipHandler::readCentralDirHeader( ObjectSet<ZipFileInfo>* zfileinfo )
{
    if ( offsetofcentraldir_ == 0 && !readEndOfCentralDirHeader() )
	return false;

    istrm_->setReadPosition( offsetofcentraldir_ );
    od_stream::Pos fileheadpos = istrm_->position();
    char headerbuff[1024];
    bool sigcheck;
    for ( int idx=0; idx<cumulativefilecounts_.last(); idx++ )
    {
	istrm_->getBin( headerbuff, mCentralHeaderSize );
	headerbuff[mCentralHeaderSize] = '\0';
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    closeInputStream();
	    mErrRet( srcfile_, " is not a valid zip archive", "" )
	}

	if ( getBitValue( *(headerbuff + mLCentralDirBitFlag), 0 ) )
	{
	    closeInputStream();
	    mErrRet( "Encrypted file::Not supported", "", "" )
	    ////TODO implement
	}

	mUnusedVar od_uint16 version =
			*mCast( od_uint16*, headerbuff + mLCentralDirVersion );

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

	istrm_->setReadPosition( fileheadpos + mCentralHeaderSize );
	const od_uint16 hfnmsz
		= *mCast(od_uint16*,headerbuff+mLFnmLengthCentral);
	BufferString headerfnm( (int)(hfnmsz+1), false );
	istrm_->getBin( headerfnm.getCStr(), hfnmsz );
	headerfnm[hfnmsz] = '\0';
	od_uint16 xtrafldlength = *mCast( od_uint16*,
                                          headerbuff + mLExtraFldLengthCentral);
	if ( xtrafldlength > 0 )
	    istrm_->getBin( headerbuff + mCentralHeaderSize, xtrafldlength );

	uncompfilesize_ = *(od_uint32*)(headerbuff+mLUnCompSizeCentral);
	compfilesize_ = *(od_uint32*)(headerbuff+mLCompSizeCentral);
	offsetoflocalheader_ = *(od_uint32*)(headerbuff+mLRelOffset);
	if ( uncompfilesize_ > m32BitSizeLimit ||
             compfilesize_ > m32BitSizeLimit ||
             offsetoflocalheader_ > m32BitSizeLimit )
	    readXtraFldForZIP64(headerbuff + mCentralHeaderSize, xtrafldlength);

	if ( zfileinfo )
	    *zfileinfo += new ZipFileInfo( headerfnm, compfilesize_,
					   uncompfilesize_,
                                           offsetoflocalheader_ );

	totalsize_ += uncompfilesize_;
	fileheadpos = fileheadpos
			+ *mCast( od_uint16*,headerbuff+mLFnmLengthCentral )
			+ *mCast( od_uint16*,headerbuff+mLExtraFldLengthCentral)
			+ *mCast( od_uint16*,headerbuff+mLFileComntLength )
			+ mCentralHeaderSize;
	istrm_->setReadPosition( fileheadpos );
    }

    istrm_->setReadPosition( 0 );
    return true;
}


bool ZipHandler::readEndOfCentralDirHeader()
{
    char sig[mSizeFourBytes];
    mEndOfCntrlDirHeaderSig( sig );
    od_stream::Pos filepos = istrm_->endPosition();
    if ( filepos == 0 )
	{ mErrRet( "Zip archive is empty", "", "" ) }

    filepos -= mEndOfDirHeaderSize;
    istrm_->setReadPosition( filepos );
    char headerbuff[mEndOfDirHeaderSize];
    istrm_->getBin( headerbuff, mSizeFourBytes );
    headerbuff[mSizeFourBytes] = '\0';
    const od_uint32* ihdrbuff = reinterpret_cast<od_uint32*>(headerbuff);
    const od_uint32* isig = reinterpret_cast<od_uint32*>(sig);
    while ( *ihdrbuff != *isig )
    {
	filepos--;
	if ( filepos <= 0 )
	    mErrRet( "Failed to unzip ", srcfile_,
		"\nZip archive is corrupt (cannot find header signature)" )

	istrm_->setReadPosition( filepos );
	istrm_->getBin( headerbuff, mSizeFourBytes );
    }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		    mEndOfDirHeaderSize-mSizeFourBytes );
    istrm_->setReadPosition( 0 );

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
    char sig[mSizeFourBytes];
    mZIP64EndOfDirLocatorHeaderSig( sig );
    istrm_->setReadPosition( 0, od_stream::End );
    od_stream::Pos filepos = istrm_->position();
    if ( filepos == 0 )
	{ mErrRet( "Zip archive is empty", "", "" ) }

    istrm_->setReadPosition( filepos
			- mEndOfDirHeaderSize - mZIP64EndOfDirLocatorSize );
    filepos = istrm_->position();
    istrm_->getBin( headerbuff, mSizeFourBytes );
    headerbuff[mSizeFourBytes] = '\0';
    const od_uint32* ihdrbuff = reinterpret_cast<od_uint32*>(headerbuff);
    const od_uint32* isig = reinterpret_cast<od_uint32*>(sig);
    while ( *ihdrbuff != *isig )
    {
	filepos--;
	if ( filepos <= 0 )
	    mErrRet( "Failed to unzip ", srcfile_,
		"\nZip archive is corrupt (cannot find header signature)" )

	istrm_->setReadPosition( filepos );
	istrm_->getBin( headerbuff, mSizeFourBytes );
    }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		      mZIP64EndOfDirLocatorSize-mSizeFourBytes );
    const char* cdoffbufptr = headerbuff + mLZIP64EndOfDirRecordOffset;
    offsetofcentraldir_ = *mCast( od_int64*, cdoffbufptr );
    const char* totdsksptr = headerbuff + mLZIP64EndOfDirLocatorTotalDisks;
    int totaldisks = *mCast( int*, totdsksptr );
    if ( totaldisks > 1 )
	mErrRet( "Failed to unzip ", srcfile_,
		"\nMultiple disk spanning of zip archive is not supported" )

    return readZIP64EndOfCentralDirRecord();
}


bool ZipHandler::readZIP64EndOfCentralDirRecord()
{
    char headerbuff[mZIP64EndOfDirRecordSize];
    char sig[mSizeFourBytes];
    mZIP64EndOfDirRecordHeaderSig( sig );
    istrm_->setReadPosition( offsetofcentraldir_ );
    istrm_->getBin( headerbuff, mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    const od_uint32* ihdrbuff = reinterpret_cast<od_uint32*>(headerbuff);
    const od_uint32* isig = reinterpret_cast<od_uint32*>(sig);
    if ( *ihdrbuff != *isig )
    { mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" ) }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		    mZIP64EndOfDirRecordSize-mSizeFourBytes );
    offsetofcentraldir_ = *mCast( od_int64*, headerbuff +
					     mLZIP64CentralDirOffset );
    cumulativefilecounts_ += *mCast( od_int64*, headerbuff +
						mLZIP64CentralDirTotalEntry );
    istrm_->setReadPosition( 0 );
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
    allfilenames_.add( destfile_ );
    if ( ret == 2 )
    {
	curfileidx_++;
	if ( curfileidx_ == cumulativefilecounts_.last() )
	    { readAndSetFileAttr(); closeInputStream(); }
	return true;
    }

    if ( ret == 0 )
	{ closeInputStream(); return false; }

    if ( !openStreamToWrite() )
	{ closeInputStream(); return false; }

    if ( compfilesize_ == 0 )
    {}
    else if ( compmethod_ == mDeflate )
    {
	if ( !doZUnCompress() )
	    { closeOutputStream(); closeInputStream(); return false; }
    }
    else if ( compmethod_ == mNoCompression )
    {
	const od_uint32 chunksize = mMIN( mMaxChunkSize, compfilesize_ );
	od_int64 count = chunksize;
	mAllocLargeVarLenArr( char, in, chunksize );
	if ( !in )
	    mErrRet("Cannot allocate memory.","System is out of memory","")

	for ( bool finish=false; !finish; )
	{
	    const od_stream::Count tohandle = count <= compfilesize_
				? chunksize : compfilesize_ % chunksize;
	    istrm_->getBin( in, tohandle );
	    ostrm_->addBin( in, tohandle );
	    if ( count > compfilesize_ )
		finish = true;

	    count += tohandle;
	    const bool inpfail = istrm_->isBad();
	    const bool outfail = ostrm_->isBad();
	    if ( inpfail || outfail )
	    {
		errormsg_.set( "Failed to unzip " ).add( istrm_->fileName() );
		if ( inpfail )
		    istrm_->addErrMsgTo( errormsg_ );
		else
		{
		    errormsg_.add( " because of a write error to " )
			     .add( ostrm_->fileName() );
		    ostrm_->addErrMsgTo( errormsg_ );
		}
		closeOutputStream(); closeInputStream();
		return false;
	    }
	}
    }
    else
    {
	closeOutputStream(); closeInputStream();
	mErrRet( "Failed to unzip ", srcfile_, "\nCompression method used \
					       is not supported" )
    }

    closeOutputStream();
    setTimeDateModified( destfile_.buf(), lastmodtime_, lastmoddate_ );

    curfileidx_++;
    if ( curfileidx_ == cumulativefilecounts_.last() )
	{ readAndSetFileAttr(); closeInputStream(); }

    return true;
}


int ZipHandler::readLocalFileHeader()
{
    char headerbuff[1024];
    od_stream::Pos filepos = istrm_->position();
    istrm_->getBin( headerbuff, mHeaderSize );
    headerbuff[mHeaderSize] = '\0';
    if ( istrm_->lastNrBytesRead() != mHeaderSize )
	return reportReadError();

    bool sigcheck;
    mFileHeaderSigCheck( headerbuff, 0 );
    if ( !sigcheck )
	mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt" )

    if ( getBitValue( *(headerbuff + mLGenPurBitFlag), 0 ) )
	mErrRet( "Encrypted file::Not supported", "", "" )
	    ////TODO implement

    mUnusedVar od_uint16 version =
			*mCast( od_uint16*, headerbuff + mLVerNeedToExtract );

    od_uint16 compmethod = *mCast( od_uint16*, headerbuff + mLCompMethod );
    if ( compmethod != mDeflate && compmethod != 0 )
	mErrRet( "Failed to unzip ", srcfile_, "\nCompression method used \
					   is not supported" )

    od_uint16 bitflag = *mCast( od_uint16*, headerbuff + mLGenPurBitFlag );
    if ( bitflag > 14 )
	mErrRet("Failed to unzip ", srcfile_,
	      "\nVersion of zip format needed to unpack is not supported")

    compmethod_ = *mCast( od_uint16*, headerbuff + mLCompMethod );
    lastmodtime_ = *mCast( od_uint16*, headerbuff + mLLastModFTime );
    lastmoddate_ = *mCast( od_uint16*, headerbuff + mLLastModFDate );
    crc_ = *mCast( od_uint32*, headerbuff + mLCRC32 );
    compfilesize_ = *mCast( od_uint32*, headerbuff + mLCompSize );
    uncompfilesize_ = *mCast( od_uint32*, headerbuff + mLUnCompSize );
    srcfnmsize_ = *mCast( od_uint16*, headerbuff + mLFnmLength );
    od_uint16 xtrafldlth = *mCast( od_uint16*, headerbuff + mLExtraFldLength );
    istrm_->getBin( headerbuff, srcfnmsize_ );
    if ( istrm_->lastNrBytesRead() != srcfnmsize_ )
	return reportReadError();

    headerbuff[srcfnmsize_] = 0;
    if ( headerbuff[srcfnmsize_ - 1] == '/' )
    {
	headerbuff[srcfnmsize_ - 1] = 0;
	destfile_ = destbasepath_;
	destfile_ += headerbuff;
#ifdef __win__
	destfile_.replace( '/', '\\' );
#endif
	if ( !File::exists(destfile_.buf()) &&
	     !File::createDir(destfile_.buf()) )
	    mErrRet("Failed to unzip ",srcfile_,"\nUnable to create directory.")

	istrm_->setReadPosition( filepos + mHeaderSize + srcfnmsize_ +
				 xtrafldlth );
	return 2;
    }

    destfile_ = destbasepath_;
    destfile_ += headerbuff;
#ifdef __win__
    destfile_.replace( '/', '\\' );
#endif
    if ( xtrafldlth > 0 && compfilesize_ > m32BitSizeLimit )
    {
	istrm_->getBin( headerbuff, xtrafldlth );
	readXtraFldForZIP64( headerbuff, xtrafldlth );
    }

    istrm_->setReadPosition( filepos + mHeaderSize + srcfnmsize_ + xtrafldlth );
    return 1;
}


bool ZipHandler::openStreamToWrite()
{
    closeOutputStream();
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

    if ( File::exists(destfile_) )
    {
	if ( !File::isWritable(destfile_) )
	    File::makeWritable( destfile_, true, false );
	if ( File::isLink(destfile_) && !File::remove(destfile_) )
	    return reportWriteError();
    }

    ostrm_ = new od_ostream( destfile_ );
    return ostrm_->isOK() ? true : reportWriteError();
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
    mAllocLargeVarLenArr( char, in, chunksize );
    mAllocLargeVarLenArr( char, out, chunksize );
    if ( !in || !out )
    { mErrRet("Cannot allocate memory.","System is out of memory","") }
    od_int64 count = chunksize;
    od_uint32 crc = 0;
    od_uint32 bytestowrite;
    int flushpolicy = Z_NO_FLUSH;
    do
    {
	const od_stream::Count tohandle = count <= compfilesize_
			    ? chunksize : compfilesize_ % chunksize;
	istrm_->getBin( in, tohandle );
	if ( count > compfilesize_ )
	    flushpolicy =  Z_FINISH ;
	count += tohandle;

	zlibstrm.avail_in = (uInt)istrm_->lastNrBytesRead();
	if ( zlibstrm.avail_in == 0 )
	    break;

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
	    ostrm_->addBin( out, bytestowrite );
	    if ( !ostrm_->isOK() )
	    {
		(void)inflateEnd( &zlibstrm );
		mErrRet( "Failed to unzip ", srcfile_, "\nError occured while \
						    writing to disk" )
	    }

	} while ( zlibstrm.avail_out == 0  );

    } while ( flushpolicy != Z_FINISH );

    inflateEnd( &zlibstrm );
    if ( !(crc == crc_) )
	mErrRet( "Failed to unzip ", srcfile_, "\nZip archive is corrupt. " )

    return ret == Z_STREAM_END ? true : false;
#else
    pErrMsg( "ZLib not available" );
    return false;
#endif
}


#define mIfFileIsSymlink \
    getBitValue(fileattr.bytes_[1],7) && getBitValue(fileattr.bytes_[1],5)


bool ZipHandler::readAndSetFileAttr()
{
    if ( offsetofcentraldir_ == 0 )
    {
	if ( !readEndOfCentralDirHeader() )
	    return false;
    }

    istrm_->setReadPosition( offsetofcentraldir_ );
    od_stream::Pos fileheadpos = istrm_->position();
    unsigned char headerbuff[1024];
    union FileAttr fileattr;
    for ( int index=0; index<allfilenames_.size(); index++ )
    {
	const BufferString& curfname = allfilenames_.get( index );
	istrm_->getBin( headerbuff, mCentralHeaderSize );
	headerbuff[mCentralHeaderSize] = '\0';
#ifdef __win__
        fileattr.bytes_[0] = headerbuff[mLExtFileAttr];
        fileattr.bytes_[1] = '\0';
        fileattr.bytes_[2] = '\0';
        fileattr.bytes_[3] = '\0';
        SetFileAttributes( curfname, fileattr.integer_ );
#else
        fileattr.bytes_[0] = headerbuff[mLExtFileAttr+mLUNIXFileAttr];
        fileattr.bytes_[1] = headerbuff[mLExtFileAttr+mLUNIXFileAttr+1];
        fileattr.bytes_[2] = '\0';
        fileattr.bytes_[3] = '\0';
        if ( mIfFileIsSymlink )
        {
	    od_istream deststrm( curfname );
            char linkbuff[1024];
	    deststrm.getBin( linkbuff, 1023 );
            linkbuff[deststrm.lastNrBytesRead()] = '\0';
            deststrm.close();
            File::remove( curfname );
            File::createLink( linkbuff, curfname );
        }
        else if ( fileattr.integer_ )
            chmod( curfname, fileattr.integer_ );
#endif
        fileheadpos = fileheadpos
			+ *mCast( od_uint16*,headerbuff+mLFnmLengthCentral )
			+ *mCast( od_uint16*,headerbuff+mLExtraFldLengthCentral)
			+ *mCast( od_uint16*,headerbuff+mLFileComntLength )
			+ mCentralHeaderSize;
	istrm_->setReadPosition( fileheadpos );
    }

    return true;
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
#ifndef OD_NO_QT
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

    const od_uint16* dosformat = reinterpret_cast<od_uint16*>(bte);
    return *dosformat;
#else
    return 0;
#endif
}


od_uint16 ZipHandler::dateInDosFormat( const char* fnm )const
{
#ifndef OD_NO_QT
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

    const od_uint16* dosformat = reinterpret_cast<od_uint16*>(bte);
    return *dosformat;
#else
    return 0;
#endif
}


bool ZipHandler::setTimeDateModified( const char* fnm, od_uint16 timeindos,
				      od_uint16 dateindos ) const
{
#ifndef OD_NO_QT
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
#else
    return false;
#endif
}


int ZipHandler::getCumulativeFileCount( int dir ) const
{
    if ( cumulativefilecounts_.validIdx(dir) )
	return cumulativefilecounts_[dir];

    return -1;
}
