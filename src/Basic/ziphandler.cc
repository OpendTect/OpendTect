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
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "executor.h"
#include "separstr.h"
#include "task.h"
#include "ziparchiveinfo.h"

#include "QFileInfo"
#include "QDateTime"
#include "QDate"
#include "QTime"

#ifdef OD_USEZLIB
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
#define mVerNeedToExtractForDir 10
#define mCompMethod 8
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
#define mLTotalEntryOnDisk 8
#define mLTotalEntry 10
#define mLSizeCentralDir 12
#define mLOffsetCentralDir 16
#define mLZipFileComntLength 20
#define mSizeTwoBits 2
#define mSizeFourBits 4


bool ZipHandler::initMakeZip( const char* destfnm, BufferStringSet srcfnm )
{
    for ( od_int32 idx=0; idx<srcfnm.size(); idx++ )
    {
	if ( File::isFile(srcfnm.get(idx).buf()) == true )
	{
	    allfilenames_.add( srcfnm.get(idx) );
	    cumulativefilecount_ += allfilenames_.size();
	}
	else if ( File::isDirectory(srcfnm.get(idx).buf()) == true )
	{
	    allfilenames_.add( srcfnm.get(idx) );
	    manageDir( srcfnm.get(idx).buf() );
	    cumulativefilecount_ += allfilenames_.size();
	}
	else
	{
	    errormsg_ = srcfnm.get(idx);
	    errormsg_ += " does not exist\0";
	    return false;
	}
    }

    FilePath fp( srcfnm.get(0).buf() );
    nrlevels_ = fp.nrLevels();
    initialfilecount_ = 0;
    destfile_ = destfnm;
    osd_ = StreamProvider( destfile_.buf() ).makeOStream( true );
    if ( !osd_.usable() )
    {
	errormsg_ = destfile_;
	errormsg_ += " Permission Denied";
	osd_.close();
	return false;
    }

    return true;
}


bool ZipHandler::manageDir( const char* src )
{

    DirList dlist( src, DirList::DirsOnly, 0 );
    DirList flist( src, DirList::FilesOnly, 0 );

    for( od_int32 idx=0; idx<flist.size(); idx++)
	allfilenames_.add( flist.fullPath(idx) );

    for( od_int32 idx=0; idx<dlist.size(); idx++ )
    {
	allfilenames_.add( dlist.fullPath(idx) );
	manageDir( dlist.fullPath(idx) );
    }

    return true;
}


od_int32 ZipHandler::openStrmToRead( const char* src )
{
    srcfile_ = src;
    if ( File::isDirectory(src) )
    {
	if ( !setLocalFileHeaderForDir() )
	    return 0;

	return 2;
    }

    if( !File::exists(src) )
    {
	errormsg_ = src;
	errormsg_ += " does not exist.\0";
	return false;
    }

    isd_ = StreamProvider( src ).makeIStream( true );
    if ( !isd_.usable() ) 
    {
	errormsg_ = "Permission to read ";
	errormsg_ += src;
	errormsg_ += " denied";
	isd_.close();
	return 0;
    }

    return 1;
}

void ZipHandler::setCompLevel( CompLevel cl )
{
    complevel_ = (od_int32) cl;
}


bool ZipHandler::doZCompress()
{
#ifdef OD_USEZLIB
    const od_uint32 ptrlocation = osd_.ostrm->tellp();
    if ( !setLocalFileHeader( ) )
	return false;

    od_int32 ret;
    z_stream strm;
    const od_int32 method = Z_DEFLATED;
    const od_int32 windowbits = -15;
    const od_int32 memlevel = 8;
    const od_int32 stategy = Z_DEFAULT_STRATEGY;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2( &strm, complevel_, method, windowbits, memlevel,
								    stategy );
    od_uint32 upperbound = deflateBound( &strm, srcfilesize_ );
    if ( ret != Z_OK ) 
    {
	errormsg_ = "Error:Compression State not initialised properly";
	return false;
    }

    destfilesize_ = 0;
    crc_ = 0;

    od_int32 flush = Z_FINISH;
    unsigned char* in = new unsigned char[srcfilesize_ + 1];    
    unsigned char* out = new unsigned char[upperbound];
    unsigned towrite;
    do
    {
	isd_.istrm->read( (char*)in , srcfilesize_ + 1 );
	strm.avail_in = isd_.istrm->gcount();
	flush = isd_.istrm->eof() ? Z_FINISH : Z_NO_FLUSH;
	crc_ = crc32( crc_, (Bytef*) in, isd_.istrm->gcount() );
	strm.next_in = in;
	do
	{
	    strm.avail_out = upperbound;
	    strm.next_out = out;
	    ret = deflate( &strm, flush );   
	    if ( ret == Z_STREAM_ERROR )
	    {
		delete [] in;
		delete [] out;
		errormsg_ = "Error: Corrupt Data ";
		return false;
	    }

	    towrite = upperbound - strm.avail_out;
	    destfilesize_ = destfilesize_ + towrite;
	    if ( towrite > 0 ) 
		osd_.ostrm->write( (const char*)out , towrite );

	    if ( osd_.ostrm->fail() )
	    {
		delete [] in;
		delete [] out;
		(void) deflateEnd ( &strm );
		errormsg_ = "Error:Writing to disk failed";
		return false;
	    }

	} while ( strm.avail_out == 0 );

    } while( flush != Z_FINISH );

    deflateEnd( &strm );
    osd_.ostrm->seekp( ptrlocation + mLCRC32 );
    osd_.ostrm->write( (const char*) &crc_, sizeof(od_uint32) );
    if ( osd_.ostrm->fail() )
    {
	delete [] in;
	delete [] out;
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    osd_.ostrm->write( (const char*) &destfilesize_, sizeof(od_uint32) );
    if ( osd_.ostrm->fail() )
    {
	delete [] in;
	delete [] out;
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    osd_.ostrm->seekp( mHeaderSize + destfilesize_ 
			    			+ srcfnmsize_ + ptrlocation );
    delete [] in;
    delete [] out;
    return true;
#else
    errormsg_ = "ZLib not available";
    return false;
#endif
}


bool ZipHandler::setLocalFileHeader()
{
    srcfilesize_ = ( od_uint32 ) File::getFileSize( srcfile_ );
    FilePath fnm( srcfile_ );
    od_int32 p = fnm.nrLevels();
    srcfnm_ = "";
    for ( od_int32 idx = ( nrlevels_ - 1 ); idx <= (p - 2); idx++ )
    {
	srcfnm_.add( fnm.dir( idx ) );
	srcfnm_ += "/";
    }

    srcfnm_.add( fnm.fileName() );
    srcfnmsize_ = ( od_uint16 ) srcfnm_.size();
    char* buf = 0;
    unsigned char headerbuff[1024];
    const od_uint16 nullvalue = 0;
    const od_uint16 version = mVerNeedToExtract;
    const od_uint16 compmethod = mCompMethod;

    mLocalFileHeaderSig ( headerbuff );
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtract, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLGenPurBitFlag, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, compmethod, mLCompMethod, mSizeTwoBits );
    if ( complevel_ == Maximum )
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
    else if ( complevel_ == Fast )
	setBitValue( headerbuff[mLGenPurBitFlag], 3, 1 );
    else if ( complevel_ == SuperFast )
    {
	setBitValue( headerbuff[mLGenPurBitFlag], 2, 1 );
	setBitValue( headerbuff[mLGenPurBitFlag], 3, 1 );
    }
    else if ( complevel_ == NoComp )
	mInsertToCharBuff( headerbuff, nullvalue, mLCompMethod, mSizeTwoBits );

    od_int16 datetime;
    datetime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, mLLastModFTime, mSizeTwoBits );
    datetime = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, mLLastModFDate, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, srcfilesize_, mLUnCompSize, mSizeFourBits );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtraFldLength, mSizeTwoBits );
    osd_.ostrm->write( (const char*) headerbuff, mHeaderSize );
    if ( osd_.ostrm->fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    osd_.ostrm->write( srcfnm_.buf(), (srcfnmsize_) );
    if ( osd_.ostrm->fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    return true;
}


bool ZipHandler::setLocalFileHeaderForDir()
{
    FilePath fnm( srcfile_ );
    od_int32 p = fnm.nrLevels();
    srcfnm_ = "";
    for ( od_int32 idx=(nrlevels_-1); idx<=(p-1); idx++ )
    {
	srcfnm_.add( fnm.dir( idx ) );
	srcfnm_ += "/";
    }

    srcfnmsize_ = ( od_uint16 ) srcfnm_.size();
    unsigned char headerbuff[1024];
    mLocalFileHeaderSig ( headerbuff );
    headerbuff[mLVerNeedToExtract] = mVerNeedToExtractForDir;
    for ( od_int32 idx=5; idx<26; idx++ )
	headerbuff[idx] = 0;

    char* buf = 0;
    od_int16 datetime;
    const od_uint16 nullvalue = 0;
    datetime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, mLLastModFTime, mSizeTwoBits );
    datetime = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, mLLastModFDate, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, srcfnmsize_, mLFnmLength, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtraFldLength, mSizeTwoBits );
    osd_.ostrm->write( (const char*) headerbuff, mHeaderSize );
    if ( osd_.ostrm->fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    osd_.ostrm->write( srcfnm_.buf(), srcfnmsize_ );
    if ( osd_.ostrm->fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    return true;
}


bool ZipHandler::setCentralDirHeader()
{
    StreamData isdest = StreamProvider( destfile_.buf() ).makeIStream( true );
    if ( !isdest.usable() )
    {
	errormsg_ =  destfile_;
	errormsg_ += " cannot be open to read";
	return false;
    }

    std::istream& readdest = *isdest.istrm;

    char headerbuff[1024];
    char* buf;
    const od_uint32 nullvalue = 0;

    mCntrlDirHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, nullvalue, mLVerMadeBy, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLFileComntLength, 
	mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLIntFileAttr, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLExtFileAttr, mSizeFourBits );
    headerbuff[mLDiskNoStart] = 1;
    headerbuff[mLDiskNoStart + 1] = 0;
    for( od_int32 index=0; index<cumulativefilecount_.last()+initialfilecount_; 
								index++ )
    {
	od_uint32 offset;
	od_uint32 compsize;
	od_uint16 fnmsize;
	char localheader[1024];
	offset = readdest.tellg();
	readdest.read( (char*)localheader, mHeaderSize);
	localheader[mHeaderSize] = 0;
	compsize = *(od_uint32*)( localheader + mLCompSize );
	fnmsize = *(od_uint16*)( localheader + mLFnmLength );
	readdest.read( (char*)(localheader+mHeaderSize),fnmsize );
	localheader[mHeaderSize + fnmsize] = 0;
	for( od_int32 id=4; id<mHeaderSize; id++ )
	    headerbuff[id + 2] = localheader[id];

	for( od_int32 id=0; id<fnmsize; id++ )
	    headerbuff[id + mCentralHeaderSize] = localheader[id + mHeaderSize];

	mInsertToCharBuff( headerbuff, offset, mLRelOffset, mSizeFourBits );
	readdest.seekg( mHeaderSize + compsize + fnmsize + offset );
	osd_.ostrm->write( (char*)headerbuff,(mCentralHeaderSize+fnmsize)  );
	if ( osd_.ostrm->fail() )
	{
	    errormsg_ = "Error:Writing to disk failed";
	    return false;
	}

    } 

    mCntrlDirDigitalSig( headerbuff, 0 );
    mInsertToCharBuff( headerbuff, nullvalue, mLSizeOfData, mSizeTwoBits );
    osd_.ostrm->write( ( char* )headerbuff, mDigSigSize  );
    if ( osd_.ostrm->fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	isdest.close();
	return false;
    }

    isdest.close();
    return true;
}


bool ZipHandler::setEndOfCentralDirHeader( od_int32 ptrlctn )
{
    char headerbuff[100];
    mEndOfCntrlDirHeaderSig( headerbuff );
    headerbuff[mLDiskNo] = 1;
    headerbuff[mLDiskNo + 1] = 0;
    headerbuff[mLCentralDirDiskNo] = 1;
    headerbuff[mLCentralDirDiskNo + 1] = 0;

    std::ostream& dest = *osd_.ostrm;
    const od_uint16 nullvalue = 0;
    od_int32 ptrlocation = dest.tellp();
    od_int32 sizecntrldir = ptrlocation - ptrlctn;
    char* buf;

    cumulativefilecount_[(cumulativefilecount_.size()-1)] = 
			  cumulativefilecount_.last() + initialfilecount_;
    mInsertToCharBuff( headerbuff, cumulativefilecount_.last(), 
					    mLTotalEntryOnDisk, mSizeTwoBits );
    mInsertToCharBuff( headerbuff, cumulativefilecount_.last(), mLTotalEntry, 
								mSizeTwoBits);
    mInsertToCharBuff( headerbuff, sizecntrldir, mLSizeCentralDir, 
								mSizeFourBits );
    mInsertToCharBuff( headerbuff, ptrlctn, mLOffsetCentralDir, 
								mSizeFourBits );
    mInsertToCharBuff( headerbuff, nullvalue, mLZipFileComntLength, 
								 mSizeTwoBits );
    dest.write( (char*) headerbuff, mEndOfDirHeaderSize );
    if ( dest.fail() )
    {
	errormsg_ = "Error:Writing to disk failed";
	return false;
    }

    return true;
}


bool ZipHandler::initUnZipArchive( const char* srcfnm, const char* basepath )
{
    if ( !File::exists(srcfnm) )
    {
	errormsg_ = srcfnm; 
	errormsg_ += " does not exist"; 
	return false; 
    }

    FilePath fp;
    srcfile_ = srcfnm;
    if ( !File::isDirectory(basepath) )
    {
	errormsg_ = basepath;
	errormsg_ += " is not a valid path";
	return false;
    }

    destbasepath_ = basepath;
    destbasepath_ += fp.dirSep( fp.Local );
    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    {
	errormsg_ = srcfnm; 
	errormsg_ += ":does not have permission to read";
	return false;
    }

    if ( !readEndOfCentralDirHeader() )
	return false;

    return true;
}


bool ZipHandler::unZipFile( const char* srcfnm, const char* fnm, 
			    const char* path )
{
    if ( !File::exists(srcfnm) )
    {
	errormsg_ = srcfnm; 
	errormsg_ += " does not exist"; 
	return false; 
    }

    ZipArchiveInfo zai( srcfnm );
    od_int64 offset = zai.getLocalHeaderOffset( fnm );
    if ( offset == -1 )
    {
	errormsg_ = zai.errorMsg();
	return false;
    }

    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() ) 
    {
	errormsg_ = srcfnm;
	errormsg_ += ": Permission to read denied";
	return false;
    }

    isd_.istrm->seekg( offset );
    srcfile_ = srcfnm;
    FilePath fp;
    fp = srcfnm;
    if ( !File::isDirectory(path) )
    {
	errormsg_ = path;
	errormsg_ += " is not a valid path";
	return false;
    }

    destbasepath_ = path;	//If destination to unzip also given
    destbasepath_ += fp.dirSep( fp.Local );
    if ( !readFileHeader() )
    {
	isd_.close();
	return false;
    }

    isd_.close();
    return true;
}


bool ZipHandler::readEndOfCentralDirHeader()
{
    char headerbuff[mEndOfDirHeaderSize];
    od_int32 ptrlocation;
    char sig[5];
    mEndOfCntrlDirHeaderSig( sig );
    isd_.istrm->seekg( 0 , std::ios::end );
    ptrlocation = isd_.istrm->tellg();
    if ( ptrlocation == 0 )
    {
	errormsg_ = "Compressed(Zipped) folder is empty";
	return false;
    }

    isd_.istrm->seekg( ptrlocation - mEndOfDirHeaderSize , std::ios::beg );
    ptrlocation = isd_.istrm->tellg();
    isd_.istrm->read( (char*) headerbuff, 4 );
    headerbuff[4] = 0;
    while (!( *( od_uint32* )( headerbuff ) == *( od_uint32* )( sig )))
    {
	isd_.istrm->seekg( (ptrlocation-1), std::ios::beg );
	ptrlocation = isd_.istrm->tellg();
	if ( ptrlocation == 0 || ptrlocation == -1 )
	{
	    errormsg_ = "Compressed(Zipped) folder is Corrupt";
	    return false;
	}

	isd_.istrm->read( (char*)headerbuff, 4 );
    }

    isd_.istrm->read( (char*)headerbuff+4, 18 );
    isd_.istrm->seekg(0);   
    cumulativefilecount_ += *( (od_int16*)(headerbuff+mLTotalEntry) );
    sizeofcentraldir_  = *( (od_int32*)(headerbuff+mLSizeCentralDir) );
    offsetofcentraldir_ = *( (od_int32*)(headerbuff+mLOffsetCentralDir) );
    commentlen_ = *( (od_int16*)(headerbuff+mLZipFileComntLength) );
    return true;
}


bool ZipHandler::readFileHeader()
{
    char headerbuff[1024];
    const od_int32 ptrlocation = isd_.istrm->tellg();
    isd_.istrm->read( (char*) headerbuff, mHeaderSize );
    headerbuff[mHeaderSize] = 0;
    if ( isd_.istrm->gcount() != mHeaderSize )
    {
	errormsg_ = "Error: Reading operation failed";
	return false;
    }

    bool sigcheck;
    mFileHeaderSigCheck( headerbuff, 0 );
    if ( !sigcheck )
    {
	errormsg_ = "Local File Header Signature not match";
	return false;
    }

    if ( getBitValue( *(headerbuff + mLGenPurBitFlag), 1 ) )
    {
	errormsg_ = "Encrypted file::Not supported";
	//TODO
	return false;
    }

    version_ = *( (od_uint16*)( headerbuff + mLVerNeedToExtract ) );
    if ( version_ > mVerNeedToExtract )
    {
	errormsg_ = "Version needed to extract not supported";
	return false;
    }

    compmethod_ = *( (od_uint16*)( headerbuff + mLCompMethod ) );
    lastmodtime_ = *( (od_uint16*)( headerbuff + mLLastModFTime ) );
    lastmoddate_ = *( (od_uint16*)( headerbuff + mLLastModFDate ) );
    crc_ = *( (od_uint32*)( headerbuff + mLCRC32 ) );
    srcfilesize_ = *( (od_uint32*)( headerbuff + mLCompSize ) );
    destfilesize_ = *( (od_uint32*)( headerbuff + mLUnCompSize ) );
    srcfnmsize_ = *( (od_uint16*)( headerbuff + mLFnmLength ) );
    xtrafldlth_ = *( (od_uint16*)( headerbuff + mLExtraFldLength ) );
    isd_.istrm->read( (char*) headerbuff, srcfnmsize_ );
    if ( isd_.istrm->gcount() != srcfnmsize_ )
    {
	errormsg_ = "Error: Reading operation failed";
	return false;
    }

    headerbuff[srcfnmsize_] = 0;
    if ( headerbuff[srcfnmsize_ - 1] == '/' )
    {
	headerbuff[srcfnmsize_ - 1] = 0;
	destfile_ = destbasepath_;
	destfile_ += headerbuff;
	if ( !File::exists( destfile_.buf() ) )
	    File::createDir( destfile_.buf() );

	isd_.istrm->seekg( ptrlocation + srcfnmsize_ + 
	    xtrafldlth_ + mHeaderSize );
	return true;
    }

    destfile_ = destbasepath_;
    destfile_ += headerbuff;
    isd_.istrm->seekg( ptrlocation + srcfnmsize_ + xtrafldlth_ + mHeaderSize );
    SeparString str( destfile_.buf(), '/' );
    BufferString pathonly = 0;
    FilePath fp = destfile_.buf();
    if ( str.size() == 1 )
	pathonly = fp.pathOnly();    
    else
    {
	for ( od_int32 idx=0; idx<str.size()-1; idx++ )
	{
	    pathonly += str[idx];
	    pathonly += fp.dirSep( fp.Local ); 
	}

    }

    if ( !File::exists( pathonly.buf() ) )
	File::createDir( pathonly.buf() );

    osd_ = StreamProvider( destfile_ ).makeOStream();
    if ( !osd_.usable() ) 
    {
	errormsg_ = "Permission denied";
	return false;
    }

    if ( compmethod_ == mCompMethod )
    {
	if ( !doZUnCompress() )
	    return false;
    }
    else if ( compmethod_ == 0 )
    {
	unsigned char* in = new unsigned char[srcfilesize_];
	isd_.istrm->read( (char*) in, srcfilesize_ );
	osd_.ostrm->write ( (char*) in, destfilesize_ );
	if ( osd_.ostrm->fail() )
	{
	    delete [] in;
	    errormsg_ = "Error:Writing to disk failed";
	    return false;
	}

	delete [] in;
	setTimeDateModified();
	osd_.close();
	return true;
    }
    else
    {
	errormsg_ = "Compression method::not supported";
	return false;
    }

    osd_.close();
    setTimeDateModified();
    return true;
}


bool ZipHandler::doZUnCompress()
{
#ifdef OD_USEZLIB
    z_stream strm;
    const od_int32 windowbits = -15;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    od_int32 ret;
    ret = inflateInit2( &strm, windowbits );
    if ( ret!=Z_OK )
    {
	errormsg_ = "Error:Initial state not initialised properly";
	return false;
    }

    unsigned char* in = new unsigned char[srcfilesize_];
    unsigned char* out = new unsigned char[destfilesize_];
    od_uint32 crc = 0;
    unsigned have;
    od_int32 flush;
    do
    {
	isd_.istrm->read( (char *)in, srcfilesize_ );
	strm.avail_in = isd_.istrm->gcount();
	flush =  Z_FINISH ;
	if (strm.avail_in == 0 ) break;
	strm.next_in = in;
	do
	{
	    strm.avail_out = destfilesize_;
	    strm.next_out = out;
	    ret = inflate( &strm, Z_NO_FLUSH );
	    switch (ret)
	    {
	    case Z_NEED_DICT:
	    case Z_DATA_ERROR:
	    case Z_MEM_ERROR:
		(void)inflateEnd( &strm );
		delete [] in;
		delete [] out;
		errormsg_ = "Error in unzipping files.";
		return false;
	    }

	    have = destfilesize_ - strm.avail_out;
	    crc = crc32( crc, (Bytef*) out, have );
	    osd_.ostrm->write( (char*)out, have );
	    if ( osd_.ostrm->fail() )
	    {
		delete [] in;
		delete [] out;
		errormsg_ = "Error:Writing to disk failed";
		return false;
	    }

	} while ( strm.avail_out == 0 );

    } while ( flush != Z_FINISH );

    if ( !(crc == crc_) )
    {
	delete [] in;
	delete [] out;
	errormsg_ = "Error:Loss of data possible. CRC not matched";
	return false;
    }

    inflateEnd( &strm );
    delete [] in;
    delete [] out;
    return ret == Z_STREAM_END ? true : false;
#else
    errormsg_ = "ZLib not available";
    return false;
#endif
}


bool ZipHandler::initAppend( const char* srcfnm, const char* fnm )
{
    if ( !File::exists(srcfnm) )
    {
	errormsg_ = srcfnm; 
	errormsg_ += " does not exist"; 
	return false; 
    }

    FilePath fp( fnm );
    nrlevels_ = fp.nrLevels();
    destfile_ = srcfnm;
    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    {
	errormsg_ = srcfnm; 
	errormsg_ += ":does not have permission to read";
	return false;
    }

    if ( !readEndOfCentralDirHeader() )
	return false;

    initialfilecount_ = cumulativefilecount_.last();
    isd_.close();
    if ( File::isFile( fnm ) )
    {
	cumulativefilecount_[ (cumulativefilecount_.size()-1) ] = 1;
	allfilenames_.add( fnm );
    }
    else if ( File::isDirectory( fnm ) )
    {
	allfilenames_.add( fnm );
	manageDir( fnm );
	cumulativefilecount_[ (cumulativefilecount_.size()-1) ] = 
							   allfilenames_.size();
    }
    else
    {
	errormsg_ = fnm;
	errormsg_ += " does not exist\0";
	return false;
    }

    osd_ = makeOStreamForAppend( srcfnm );
    if ( !osd_.usable() )
    {
	errormsg_ = srcfnm;
	errormsg_ += " Permission to write to file denied";    //TO Check
	osd_.close();
	return false;
    }

    osd_.ostrm->seekp( offsetofcentraldir_ );
    return true;
}


bool ZipHandler::getArchiveInfo( const char* srcfnm, 
    ObjectSet<ZipFileInfo>& zfileinfo )
{
    if ( !File::exists(srcfnm) )
    {
	errormsg_ = srcfnm;
	errormsg_ += ": File does not exist";
	return false;
    }

    isd_ = StreamProvider( srcfnm ).makeIStream();
    if ( !isd_.usable() )
    {
	errormsg_ = srcfnm;
	errormsg_ += ": Access denied";
	return false;
    }

    if ( !readEndOfCentralDirHeader() )
	return false;

    isd_.istrm->seekg( offsetofcentraldir_, std::ios::beg );
    od_uint32 ptrlocation;
    ptrlocation = isd_.istrm->tellg();
    BufferString headerbuff;
    bool sigcheck;
    for ( od_int32 idx=0; idx<cumulativefilecount_.last(); idx++ )
    {
	isd_.istrm->read( headerbuff.buf(), mCentralHeaderSize);
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    isd_.close();
	    errormsg_ = srcfnm;
	    errormsg_ += ": File is not a valid zip archive";
	    return false;
	}

	isd_.istrm->seekg( ptrlocation + mCentralHeaderSize );
	BufferString headerfnm;
	isd_.istrm->read( headerfnm.buf(),
	    *((od_int16*)(headerbuff.buf()+mLFnmLengthCentral)) );
	headerfnm[*((od_int16*)(headerbuff.buf()+mLFnmLengthCentral))] = '\0';
	ZipFileInfo* fi = new ZipFileInfo( headerfnm, 
	    *((od_uint32*)(headerbuff.buf()+mLCompSizeCentral)),
	    *((od_uint32*)(headerbuff.buf()
	    +mLUnCompSizeCentral)),
	    *((od_uint32*)(headerbuff.buf()+mLRelOffset)) );
	zfileinfo += fi;
	ptrlocation = ptrlocation
	    + *( (od_int16*)(headerbuff.buf()+mLFnmLengthCentral) )
	    + *( (od_int16*)(headerbuff.buf()+mLExtraFldLengthCentral))
	    + *( (od_int16*)(headerbuff.buf()+mLFileComntLength) )
	    + mCentralHeaderSize;
	isd_.istrm->seekg( ptrlocation );
    }

    isd_.close();
    return true;
}


bool ZipHandler::getBitValue(const unsigned char byte, 
						    od_int32 bitposition) const
{
    unsigned char modfbyte;
    modfbyte = byte >> ( bitposition );
    if ( modfbyte % 2 == 0)
	return false;

    return true;
}


void ZipHandler::setBitValue(unsigned char& byte,
					 od_int32 bitposition, bool value) const
{
    unsigned char var = (unsigned char)  pow( 2.0 , (bitposition) );
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
    od_int32 sec = ftime.second();
    char min = ( char ) ftime.minute();
    char hr = (char) ftime.hour();
    sec = sec/2;
    bte[0] = (char) sec;
    bte[1] = 0;
    od_int32 idx;
    for ( idx=5; idx<8; idx++ )
	setBitValue( bte[0], idx, getBitValue(min,idx-5) );

    for ( idx=0; idx<3; idx++ )
	setBitValue( bte[1], idx, getBitValue(min,idx+3) );

    for ( idx=3; idx<8; idx++ )
	setBitValue( bte[1], idx, getBitValue(hr,idx-3) );

    od_uint16 dosformat;
    dosformat = *( ( od_uint16* ) ( bte ) );
    return dosformat;
}


od_int16 ZipHandler::dateInDosFormat( const char* fnm )const
{
    unsigned char bte[2];
    QFileInfo qfi( fnm );
    QDate fdate = qfi.lastModified().date();
    unsigned char day = ( char ) fdate.day();
    unsigned char month = ( char ) fdate.month();
    od_int32 year = fdate.year();
    unsigned char dosyear;
    dosyear = ( unsigned char ) (year - 1980);
    bte[0] =  day;
    bte[1] = 0;
    od_int32 idx;
    for ( idx = 5; idx < 8; idx++ )
	setBitValue( bte[0], idx, getBitValue(month,idx-5) );

    for ( idx = 0; idx < 1; idx++ )
	setBitValue( bte[1], idx, getBitValue(month,idx+3) );

    for ( idx = 1; idx < 8; idx++ )
	setBitValue( bte[1], idx, getBitValue(dosyear,idx-1) );

    od_uint16 dosformat;
    dosformat = *( ( od_uint16* ) ( bte ) );
    return dosformat;
}


bool ZipHandler::setTimeDateModified()
{
    unsigned char bytetime[2], bytedate[2], byte = 0;
    od_int32 sec, min, hour, day, month, year, idx;
    bytetime[0] = *( (char*)( &lastmodtime_) );
    bytetime[1] = *( ((char*)( &lastmodtime_))+1 );
    bytedate[0] = *( (char*)( &lastmoddate_) );
    bytedate[1] = *( ((char*)(&lastmoddate_))+1 );
    for ( idx=0; idx<5; idx++ )
	setBitValue( byte, idx, getBitValue(bytetime[0],idx) );

    byte = byte * 2;
    sec = byte;
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
    if ( _utime( destfile_.buf(), &ut) == -1 )
	return false;
#else
    struct utimbuf ut;
    ut.modtime = timeinsec;
    if ( utime( destfile_.buf(), &ut) == -1 )
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


od_int32 ZipHandler::getCumulativeFileCount( od_int32 dir ) const
{
    if ( cumulativefilecount_.validIdx(dir) )
	return cumulativefilecount_[dir];

    return -1;
}
