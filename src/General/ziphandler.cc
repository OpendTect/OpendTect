/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ziphandler.h"

#include "bufstring.h"
#include "databuf.h"
#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "od_iostream.h"
#include "ptrman.h"
#include "separstr.h"
#include "task.h"
#include "timefun.h"
#include "uistrings.h"
#include "varlenarray.h"
#include "ziparchiveinfo.h"

#ifndef OD_NO_QT
# include <QDate>
# include <QFileInfo>
# include <QTimeZone>
#endif

#ifdef HAS_ZLIB
# include "zlib.h"
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

#define mEndOfDirHeaderSize 22

#define mLDOSFileAttr 0
#define mLUNIXFileAttr 2
#define mVerNeedToExtract 45
#define mVersionZip 63 // 6.3

#define mLVerNeedToExtract 4
#define mLGenPurBitFlag 6
#define mLCompMethod 8
#define mLLastModFTime 10
#define mLLastModFDate 12
#define mLCRC32 14
#define mLCompSize 18
#define mLUnCompSize 22
#define mLFnmLength 26
#define mLExtraFldLength 28
#define mLHeaderSize 30			//L for Local file header

#define mLVerMadeBy 4
#define mLOSMadeBy 5
#define mLVerNeedToExtractCentral 6
#define mLGenPurBitFlagCentral 8
#define mLCompMethodCentral 10
#define mLastModFTimeCentral 12
#define mLastModFDateCentral 14
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
#define mCentralHeaderSize 46

#define mLSizeOfData 4
#define mLDiskNo 4
#define mLCentralDirVersion 6
#define mLCentralDirBitFlag 8
#define mLCentralDirCompMethod 10
#define mLCentralDirDiskNo 6
#define mLTotalEntryOnDisk 8
#define mLTotalEntry 10
#define mLSizeCentralDir 12
#define mLOffsetCentralDir 16
#define mLZipFileComntLength 20

#define mZ_NOCOMP 0
#define mSizeOneByte 1
#define mSizeTwoBytes 2
#define mSizeThreeBytes 3
#define mSizeFourBytes 4
#define mSizeEightBytes 8
#define mMaxChunkSize 10485760	     //10MB
#define m16BitDiskLimit 0xFFFE
#define m32BitSizeLimit mDef4GB
#define mZIP64DiskLimit 0xFFFF
#define mZIP64SizeLimit 0xFFFFFFFF
#define mZIP64EndOfDirRecordSize 56
#define mZIP64EndOfDirLocatorSize 20
#define mLZIP64EndOfDirRecordOffset 8
#define mLZIP64EndOfDirLocatorTotalDisks 16
#define mLZIP64CentralDirOffset 48
#define mLZIP64CentralDirTotalEntry 32
#define mZIP64Tag 0x0001
#define mNTFSTag 0x000a		      // "\n"
#define mExtTimeStampTag 0x5455       // "UT"
#define mInfoZipUnixTag 0x5855	      // "UX"
#define mInfoZipPrevNewUnixTag 0x7855 // "Ux"
#define mInfoZipNewUnitTag 0x7875     // "ux"
#define mExtraFieldTagPos 0
#define mExtraFieldTSizePos 2
#define mSizeOfNTFSHeader 32
#define mSizeOfUIDGIDHeader 15
#define mExtTimeStampFlagPos 4
#define mExtTimeStampModTimePos 5
#define mExtTimeStampAccTimePos 9
#define mExtTimeStampCrTimePos 13
#define mUIDGIDVerPos 4
#define mUIDSizePos 5
#define mUIDPos 6
#define mGIDSizePos 10
#define mGIDPos 11

#define mIsError 0
#define mIsFile 1
#define mIsDirectory 2
#define mIsLink 3
#define mZDefaultMemoryLevel 9
#define mMaxWindowBitForRawDeflate -15


namespace OD
{

static bool getBitValue( const unsigned char byte, int bitposition )
{
    unsigned char modfbyte;
    modfbyte = byte >> ( bitposition );
    if ( modfbyte % 2 == 0)
	return false;

    return true;
}


static void setBitValue( unsigned char& byte, int bitposition, bool value )
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

} // namespace OD


// ZipFileInfo

ZipFileInfo::ZipFileInfo()
    : times_(*new Time::FileTimeSet)
    , type_(*new File::Type(File::Type::Unknown))
    , perms_(*new File::Permissions())
{
}


ZipFileInfo::ZipFileInfo( const ZipFileInfo& oth )
    : times_(*new Time::FileTimeSet)
    , type_(*new File::Type(File::Type::Unknown))
    , perms_(*new File::Permissions())
{
    *this = oth;
}


ZipFileInfo::~ZipFileInfo()
{
    delete &times_;
    delete &type_;
    delete &perms_;
}


ZipFileInfo& ZipFileInfo::operator =( const ZipFileInfo& oth )
{
    if ( &oth == this )
	return *this;

    compmethod_ = oth.compmethod_;
    times_ = oth.times_;
    crc_ = oth.crc_;
    compsize_ = oth.compsize_;
    uncompsize_ = oth.uncompsize_;
    localheaderoffset_ = oth.localheaderoffset_;
    uid_ = oth.uid_;
    gid_ = oth.gid_;
    binary_ = oth.binary_;
    comment_ = oth.comment_;
    hasutcheader_ = oth.hasutcheader_;
    type_ = oth.type_;
    perms_ = oth.perms_;
    fullfnm_ = oth.fullfnm_;
    filenminzip_ = oth.filenminzip_;
    linkvalue_ = oth.linkvalue_;
    sourceiswin_ = oth.sourceiswin_;
    nrlevels_ = oth.nrlevels_;

    return *this;
}


bool ZipFileInfo::operator >( const ZipFileInfo& oth ) const
{
    return nrlevels_ > oth.nrlevels_;
}


const char* ZipFileInfo::getFileName() const
{
    return filenminzip_.buf();
}


const char* ZipFileInfo::getFullFileName() const
{
    return fullfnm_.buf();
}


const char* ZipFileInfo::getLinkValue() const
{
    return linkvalue_.buf();
}


bool ZipFileInfo::isDirectory() const
{
    return type_ == File::Type::Directory;
}


bool ZipFileInfo::isSymbolicLink() const
{

    return type_ == File::Type::SymLink;
}


bool ZipFileInfo::isWritable() const
{
    return perms_.isWritable();
}


bool ZipFileInfo::isHidden() const
{
    return perms_.isHidden();
}


bool ZipFileInfo::isSystem() const
{
    return perms_.isSystem();
}


od_uint16 ZipFileInfo::getHeaderLength( bool local ) const
{
    od_uint16 ret = 0;
    if ( needZIP64(local) )
	ret += getZIP64Size( local );

    const int timestamphdrlength = getTimeStampHeaderLength( local );
    if ( timestamphdrlength > mExtTimeStampModTimePos )
	ret += timestamphdrlength;

    if ( local )
	return ret;

    ret += 2*mSizeTwoBytes + mSizeOfNTFSHeader;
    if ( hasUIDGID() )
	ret += mSizeOfUIDGIDHeader;

    return ret;
}


bool ZipFileInfo::hasModTime() const
{
    return times_.hasModificationTime();
}


bool ZipFileInfo::hasUIDGID() const
{
    return !mIsUdf(uid_) && !mIsUdf(gid_);
}


bool ZipFileInfo::needZIP64( bool local ) const
{
    if ( uncompsize_ > m32BitSizeLimit )
	return true;

    if ( local )
	return false;

    return localheaderoffset_ > m32BitSizeLimit;
}


od_uint16 ZipFileInfo::getZIP64Size( bool local ) const
{
    od_uint16 ret = 2*mSizeTwoBytes;
    if ( local && uncompsize_ > m32BitSizeLimit )
	ret += 2*mSizeEightBytes;
    else if ( !local )
    {
	if ( uncompsize_ > m32BitSizeLimit )
	    ret += mSizeEightBytes;

	if ( compsize_ > m32BitSizeLimit )
	    ret += mSizeEightBytes;
    }

    if ( local )
	return ret;

    if ( localheaderoffset_ > m32BitSizeLimit )
	ret += mSizeEightBytes;

    return ret;
}


od_uint16 ZipFileInfo::getDosNrDays() const
{
    const std::timespec modtime = times_.getModificationTime();
    const od_int64 modtimems = Time::toMSecs( modtime );
    const QDateTime qdt = QDateTime::fromMSecsSinceEpoch( modtimems );
    const QDate fdate = qdt.date();
    const unsigned char day = mCast( char, fdate.day() );
    const unsigned char month = mCast( char, fdate.month() );
    const int year = fdate.year();
    const unsigned char dosyear = mCast( unsigned char, (year - 1980) );

    unsigned char bte[2];
    bte[0] =  day;
    bte[1] = 0;
    int idx;
    for ( idx = 5; idx < 8; idx++ )
	OD::setBitValue( bte[0], idx, OD::getBitValue(month,idx-5) );

    for ( idx = 0; idx < 1; idx++ )
	OD::setBitValue( bte[1], idx, OD::getBitValue(month,idx+3) );

    for ( idx = 1; idx < 8; idx++ )
	OD::setBitValue( bte[1], idx, OD::getBitValue(dosyear,idx-1) );

    const od_uint16* dosformat = reinterpret_cast<od_uint16*>(bte);
    return *dosformat;
}


od_uint16 ZipFileInfo::getDosNrSec() const
{
    const std::timespec modtime = times_.getModificationTime();
    const od_int64 modtimems = Time::toMSecs( modtime );
    const QDateTime qdt = QDateTime::fromMSecsSinceEpoch( modtimems );
    const QTime ftime = qdt.time();
    int sec = ftime.second();
    const char min = mCast( char, ftime.minute() );
    const char hr = mCast( char, ftime.hour() );
    sec = sec/2;

    unsigned char bte[2];
    bte[0] = mCast( char, sec );
    bte[1] = 0;
    int idx;
    for ( idx=5; idx<8; idx++ )
	OD::setBitValue( bte[0], idx, OD::getBitValue(min,idx-5) );

    for ( idx=0; idx<3; idx++ )
	OD::setBitValue( bte[1], idx, OD::getBitValue(min,idx+3) );

    for ( idx=3; idx<8; idx++ )
	OD::setBitValue( bte[1], idx, OD::getBitValue(hr,idx-3) );

    const od_uint16* dosformat = reinterpret_cast<od_uint16*>(bte);
    return *dosformat;
}


int ZipFileInfo::getTimeStampHeaderLength( bool local ) const
{
    int ret = 2*mSizeTwoBytes + mSizeOneByte;
    if ( times_.hasModificationTime() )
	ret += mSizeFourBytes;

    if ( local && times_.hasAccessTime() )
	ret += mSizeFourBytes;

    return ret;
}


void ZipFileInfo::setFileName( const char* fnm, const FilePath& basepath )
{
    filenminzip_.set( fnm );
    nrlevels_ = FilePath( filenminzip_ ).nrLevels();
    fullfnm_.setEmpty();
    if ( basepath.isEmpty() )
	return;

    FilePath fp( basepath, fnm );
    fullfnm_ = fp.fullPath();
    if ( perms_.isWindowsAttr() && !sourceiswin_ )
	perms_.setHidden( fp.fileName().startsWith(".") );
}


void ZipFileInfo::setFullFileName( const char* fnm, const FilePath& basepath )
{
    fullfnm_.set( fnm );
    sourceiswin_ = __iswin__;
    type_ = File::getType( fnm );
    perms_ = File::getPermissions( fnm );
    setFileNameInZip( basepath );
    nrlevels_ = FilePath( getFileName() ).nrLevels();
    if ( isDirectory() ||  isSymbolicLink() )
	compmethod_ = mZ_NOCOMP;
    else
	compmethod_ = Z_DEFLATED;

    if ( isDirectory() )
    {
	crc_ = 0;
	compsize_ = 0;
	uncompsize_ = 0;
    }
    else if ( isSymbolicLink() )
    {
	linkvalue_.setEmpty();
#ifdef __win__
	HANDLE filehandle = CreateFile ( fnm, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	const od_int32 linksize = GetFileSize( filehandle, NULL );
	od_int32 bytesread;
	linkvalue_.setMinBufSize( linksize );
	ReadFile( filehandle, linkvalue_.getCStr(), linksize,
		  (LPDWORD)&bytesread, NULL );
	CloseHandle( filehandle );
#else
	linkvalue_.setMinBufSize( 1024 );
	const int len = readlink( fnm, linkvalue_.getCStr(),
				       linkvalue_.bufSize() );
	if ( len >= 0 )
	    linkvalue_.getCStr()[len] = '\0';
	else
	    linkvalue_.setEmpty();

	const od_uint32 linksize = linkvalue_.size();
#endif
	crc_ = crc32( 0, mCast(Bytef*,linkvalue_.buf() ), linksize );
	compsize_ = linksize;
	uncompsize_ = linksize;
    }
    else
    {
	uncompsize_ = File::getFileSize( fnm, false );
    }

    File::getTimes( fnm, times_, false );
    if ( perms_.isWindowsAttr() )
	return;

    const QFileInfo qfinfo( fnm );
    const uint uid = qfinfo.ownerId();
    const uint gid = qfinfo.groupId();
    if ( uid == mCast(uint,-2) || gid == mCast(uint,-2) )
	return;

    uid_ = uid;
    gid_ = gid;
}


void ZipFileInfo::writeAttrToBuffer( unsigned char* databuf ) const
{
    if ( fullfnm_.isEmpty() )
	return;

    char* buf = nullptr;
    if ( perms_.isWindowsAttr() )
    {
	const od_uint32 perms = int ( perms_.asInt() );
	mInsertToCharBuff( databuf, perms, mLDOSFileAttr, mSizeFourBytes );
    }
    else
    {
	OD::setBitValue( databuf[mLDOSFileAttr], 0, !isWritable() );
	OD::setBitValue( databuf[mLDOSFileAttr], 4, isDirectory() );
	OD::setBitValue( databuf[mLDOSFileAttr], 5, !isDirectory() );
	const od_uint16 perms = od_uint16 ( perms_.get_st_mode(type_) );
	mInsertToCharBuff( databuf, perms, mLUNIXFileAttr, mSizeTwoBytes );
    }
}


void ZipFileInfo::setAttr( const unsigned char* buf, bool fromwin )
{
    if ( OD::getBitValue(buf[mLDOSFileAttr],4) )
	type_ = File::Type::Directory;
    else if ( !fromwin &&
	      OD::getBitValue(buf[mLUNIXFileAttr+1],5) &&
	      OD::getBitValue(buf[mLUNIXFileAttr+1],7) )
	type_ = File::Type::SymLink;
    else
	type_ = File::Type::File;

    /* Copy all possible permissions/file attributes from the
       initial OS to the destination OS */
    sourceiswin_ = fromwin;
    if ( sourceiswin_ )
    {
	const int permsi = *(od_uint32*)buf;
	const File::Permissions fileperms( permsi, sourceiswin_ );
	if ( __iswin__ )
	{
	    //Full preservation: Windows to Windows
	    perms_ = fileperms;
	}
	else
	{
	    // Windows to Unix (Archive, Hidden, System attribs not restored)
	    perms_ = File::Permissions::getDefault( !isDirectory() &&
						    !isSymbolicLink(), false );
	    if ( !isSymbolicLink() )
	    {
		const bool writable = fileperms.isWritable();
		perms_.setFlag( File::Permission::WriteOwner, writable )
		      .setFlag( File::Permission::WriteUser, writable );
		if ( !writable )
		{
		    perms_.setFlag( File::Permission::WriteGroup, writable )
			  .setFlag( File::Permission::WriteOther, writable );
		}
	    }
	}
    }
    else
    {
	const int permsi = *(od_uint16*)(buf+mLUNIXFileAttr);
	if ( __iswin__ )
	{
	    /* Unix to Windows (No system attribute to restore,
				Hidden attrib set by file name) */
	    const bool isreadonly = OD::getBitValue( buf[mLDOSFileAttr], 0 );
	    perms_ = File::Permissions::getDefault( !isDirectory() &&
						    !isSymbolicLink(), true );
	    perms_.setReadOnly( isreadonly );
	}
	else
	{
	    //Full preservation: Unix to Unix
	    perms_ = File::Permissions::getFrom( permsi, mUdf(int) );
	}
    }
}


void ZipFileInfo::setFileNameInZip( const FilePath& basepath )
{
    const FilePath srcfp( fullfnm_ );
    if ( !basepath.isEmpty() &&
	 (srcfp.isSubDirOf(basepath) || srcfp == basepath) )
	filenminzip_ = srcfp.fileFrom( basepath.nrLevels(), FilePath::Unix );
    else
	filenminzip_ = srcfp.fullPath( FilePath::Unix ).buf()+1;

    if ( type_ == File::Type::Directory )
	filenminzip_.add( FilePath::dirSep(FilePath::Unix) );
}


void ZipFileInfo::setDosTimeDateModified( od_uint16 dosnrdays,
					  od_uint16 dosnrsec )
{
    if ( dosnrdays == 0 || dosnrsec == 0 )
	return;

    unsigned char bytetime[mSizeTwoBytes], bytedate[mSizeTwoBytes],
		  byte = '\0';
    int sec, min, hour, day, month, year, idx;
    bytedate[0] = *mCast( char*, &dosnrdays );
    bytedate[1] = *( mCast(char*,&dosnrdays) + 1 );
    bytetime[0] = *mCast( char*, &dosnrsec );
    bytetime[1] = *( mCast(char*,&dosnrsec) + 1 );
    for ( idx=0; idx<5; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytetime[0],idx) );

    sec = byte * 2;
    byte = 0;
    for ( idx=0; idx<3; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytetime[0],idx+5) );

    for ( idx=3; idx<6; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytetime[1],idx-3) );

    min = byte;
    byte = 0;
    for ( idx=0; idx<5; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytetime[1],idx+3) );

    hour = byte;
    byte = 0;
    for ( idx=0; idx<5; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytedate[0],idx) );

    day = byte;
    byte = 0;
    for ( idx=0; idx<3; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytedate[0],idx+5) );

    for ( idx=3; idx<4; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytedate[1],idx-3) );

    month = byte;
    byte = 0;
    for ( idx=0; idx<7; idx++ )
	OD::setBitValue( byte, idx, OD::getBitValue(bytedate[1],idx+1) );

    year = byte + 1980;
    const QTime qt( hour, min, sec ); //Never accounting for time zone
    const QDate qd( year, month, day );
    const QDateTime qdt( qd, qt );
    std::timespec modtime;
    modtime.tv_sec = qdt.toSecsSinceEpoch();
    modtime.tv_nsec = 0;
    times_.setModificationTime( modtime );
}


// ZipHandler

ZipHandler::ZipHandler()
{
}


ZipHandler::~ZipHandler()
{
    if ( istrm_ )
	{ pErrMsg( "istrm_ still open" ); }
    if ( ostrm_ )
	{ pErrMsg( "ostrm_ still open" ); }

    delete istrm_;
    delete ostrm_;
}


void ZipHandler::closeInputStream()
{
    deleteAndNullPtr( istrm_ );
}


void ZipHandler::closeOutputStream()
{
    deleteAndNullPtr( ostrm_ );
}


bool ZipHandler::reportWriteError( const char* filenm ) const
{
    BufferString fnm;
    if ( filenm )
	fnm.set( filenm );
    else if ( ostrm_ )
	fnm = ostrm_->fileName();
    else
	return false;

    errormsg_ = uiStrings::phrCannotWrite( fnm.buf() );
    if ( ostrm_ )
	ostrm_->addErrMsgTo( errormsg_ );

    return false;
}


bool ZipHandler::reportReadError( const char* filenm ) const
{
    BufferString fnm;
    if ( filenm )
	fnm.set( filenm );
    else if ( istrm_ )
	fnm = istrm_->fileName();
    else
	return false;

    return reportStrmReadError( istrm_, fnm.buf() );
}


bool ZipHandler::reportStrmReadError( od_istream* strm,
				      const char* filenm ) const
{
    errormsg_ = uiStrings::phrCannotRead( filenm );
    if ( strm )
	strm->addErrMsgTo( errormsg_ );

    return false;
}


bool ZipHandler::initMakeZip( const BufferStringSet& srcfnms,
			      const char* basepath, const char* zipfilenm )
{
    totalsize_ = 0;
    basepath_.set( basepath );
    for ( int idx=0; idx<srcfnms.size(); idx++ )
    {
	const char* srcfnm = srcfnms.get(idx).buf();
	const File::Type type = File::getType( srcfnm );
	if ( type == File::Type::Directory )
	{
	    allfilenames_.add( srcfnm );
	    getFileList( srcfnm, allfilenames_ );
	    cumulativefilecounts_ += allfilenames_.size();
	}
	else if ( File::exists(srcfnm) )
	{
	    allfilenames_.add( srcfnm );
	    cumulativefilecounts_ += allfilenames_.size();
	    totalsize_ += File::getFileSize( srcfnm, false );
	}
	else
	{
	    errormsg_ = uiStrings::phrFileDoesNotExist( srcfnm );
	    return false;
	}
    }

    initialfilecount_ = 0;
    nrdonesize_ = 0;
    delete ostrm_;
    ostrm_ = new od_ostream( zipfilenm );

    return ostrm_->isOK() ? true : reportWriteError( zipfilenm );
}


bool ZipHandler::initAppend( const char* fnm, const char* basepath,
			     const char* zipfilenm )
{
    if ( !File::exists(zipfilenm) )
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( zipfilenm );
	return false;
    }

    basepath_.set( basepath );
    delete istrm_;
    istrm_ = new od_istream( zipfilenm );
    if ( istrm_->isBad() )
	return reportReadError( zipfilenm );

    if ( !readEndOfCentralDirHeader() )
	return false;

    closeInputStream();

    initialfilecount_ = cumulativefilecounts_.last();
    totalsize_ = 0;
    const File::Type type = File::getType( fnm );
    if ( type == File::Type::Directory )
    {
	allfilenames_.add( fnm );
	getFileList( fnm, allfilenames_ );
	cumulativefilecounts_[ (cumulativefilecounts_.size()-1) ] =
							   allfilenames_.size();
    }
    else if ( File::exists(fnm) )
    {
	cumulativefilecounts_[ (cumulativefilecounts_.size()-1) ] = 1;
	allfilenames_.add( fnm );
	totalsize_ += File::getFileSize( fnm, false );
    }
    else
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( fnm );
	return false;
    }

    delete ostrm_;
    ostrm_ = new od_ostream( zipfilenm, true ); // open for edit
    if ( !ostrm_->isOK() )
	return reportWriteError( zipfilenm );

    ostrm_->setWritePosition( offsetofcentraldir_ );
    nrdonesize_ = 0;
    return true;
}


bool ZipHandler::getFileList( const char* src,
			      BufferStringSet& filenames )
{
    File::makeRecursiveFileList( src, filenames );
    for( int idx=0; idx<filenames.size(); idx++ )
	totalsize_ += File::getFileSize( filenames.get(idx), false );

    return true;
}


bool ZipHandler::compressNextFile( ZipFileInfo& fileinfo )
{
    if ( curfileidx_ == getCumulativeFileCount(curinputidx_) )
	curinputidx_++;

    int ret;
    const BufferString srcfile = allfilenames_.get( curfileidx_ );
    ret = openStrmToRead( srcfile.buf() );
    if ( ret == mIsError )
    {
	closeOutputStream();
	return false;
    }

    fileinfo.setFullFileName( srcfile.buf(), basepath_ );
    fileinfo.localheaderoffset_ = ostrm_->position();
    switch( ret )
    {
    case mIsFile:
	ret = doZCompress( fileinfo );
	closeInputStream();
	break;
    case mIsDirectory:
	ret = setLocalFileHeaderForDir( fileinfo );
	break;
    case mIsLink:
	ret = setLocalFileHeaderForLink( fileinfo );
	if ( ret != mIsError )
	    ostrm_->add( fileinfo.getLinkValue() );
	break;
    default:
	break;
    }

    if ( ret == mIsError )
    {
	closeOutputStream();
	return false;
    }

    curfileidx_++;
    return true;
}


int ZipHandler::openStrmToRead( const char* src )
{
    if ( File::isSymLink(src) )
	return mIsLink;

    if ( File::isDirectory(src) )
	return mIsDirectory;

    if( !File::exists(src) )
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( src );
	return mIsError;
    }

    delete istrm_;
    istrm_ = new od_istream( src );
    if ( istrm_->isBad() )
    {
	errormsg_ = uiStrings::phrCannotOpenForRead( src );
	istrm_->addErrMsgTo( errormsg_ );
	return mIsError;
    }

    return mIsFile;
}


void ZipHandler::setCompLevel( CompLevel cl )
{
    complevel_ = cl;
}


bool ZipHandler::doZCompress( ZipFileInfo& fileinfo )
{
#ifdef HAS_ZLIB
    od_uint32& crc = fileinfo.crc_;
    od_uint64& compfilesize = fileinfo.compsize_;
    const od_uint64& uncompfilesize = fileinfo.uncompsize_;
    const od_stream::Pos initialpos = ostrm_->position();
    if ( !setLocalFileHeaderForFile(fileinfo) )
	return false;
    else if ( uncompfilesize == 0 )
	return true;

    z_stream zlibstrm;
    zlibstrm.zalloc = Z_NULL;
    zlibstrm.zfree = Z_NULL;
    zlibstrm.opaque = Z_NULL;
    int ret = deflateInit2( &zlibstrm, complevel_, fileinfo.compmethod_,
	mMaxWindowBitForRawDeflate, mZDefaultMemoryLevel, Z_DEFAULT_STRATEGY );

    const od_uint32 chunksize = mMaxLimited( uncompfilesize, mMaxChunkSize );
    od_uint32 upperbound = deflateBound( &zlibstrm, chunksize );
    if ( ret != Z_OK )
    {
	errormsg_ = tr("Error Details: Initialization required to compress "
		       "data fails" );
	errormsg_.appendPhrase( tr("Error type: %1").arg(ret) );
	return false;
    }

    mDeclareAndTryAlloc( unsigned char*, in, unsigned char[chunksize] );
    mDeclareAndTryAlloc( unsigned char*, out, unsigned char[upperbound] );
    if ( !in || !out )
    {
	delete [] in; delete [] out;
	errormsg_ = uiStrings::phrCannotAllocateMemory( chunksize+upperbound );
	return false;
    }

    compfilesize = 0; crc = 0;
    for ( int flushpolicy=Z_NO_FLUSH; flushpolicy!=Z_FINISH;  )
    {
	istrm_->getBin( in, chunksize );
	uInt nrbytesread = (uInt)istrm_->lastNrBytesRead();
	zlibstrm.avail_in = nrbytesread;
	nrdonesize_ += zlibstrm.avail_in;
	flushpolicy = istrm_->isOK() ? Z_NO_FLUSH : Z_FINISH;
	crc = crc32( crc, mCast(Bytef*,in), nrbytesread );
	zlibstrm.next_in = mCast( Bytef*, in );

	zlibstrm.avail_out = 0;
	while ( zlibstrm.avail_out == 0 )
	{
	    zlibstrm.avail_out = upperbound;
	    zlibstrm.next_out = mCast( Bytef*, out );
	    ret = deflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 )
	    {
		delete [] in; delete [] out;
		(void)deflateEnd(&zlibstrm);
		errormsg_ = tr("Failed to zip '%1'").arg( istrm_->fileName() );
		errormsg_.appendPhrase( tr("File may contain corrupt data") );
		return false;
	    }

	    const od_uint32 bytestowrite = upperbound - zlibstrm.avail_out;
	    compfilesize += bytestowrite;
	    if ( bytestowrite > 0 )
	    {
		if ( !ostrm_->addBin(out,bytestowrite) )
		{
		    delete [] in; delete [] out;
		    (void)deflateEnd(&zlibstrm);
		    return reportWriteError();
		}
	    }
	}
    }

    delete [] in; delete [] out;
    deflateEnd( &zlibstrm );

    //Update some local header values:
    ostrm_->setWritePosition( initialpos + mLCRC32 );
    ostrm_->addBin( &crc, sizeof(od_uint32) );
    const bool needzip64 = fileinfo.needZIP64( true );
    if ( !needzip64 )
	ostrm_->addBin( &compfilesize, sizeof(od_uint32) );

    const StringView srcfnm( fileinfo.getFileName() );
    ostrm_->setWritePosition( initialpos + mLHeaderSize + srcfnm.size() );
    if ( needzip64 )
    { //forced to rewrite all headers, too dangerous to guess where to write
	if ( !setXtraHeaders(fileinfo,true) )
	    return reportWriteError();
    }
    else
    {
	const od_uint16 xtrafldlength = fileinfo.getHeaderLength( true );
	ostrm_->setWritePosition( xtrafldlength, od_stream::Rel );
    }

    ostrm_->setWritePosition( compfilesize, od_stream::Rel );
    ostrm_->flush();
    return true;
#else
    pErrMsg( "ZLib library not available" );
    return false;
#endif
}


void ZipHandler::writeGeneralPurposeFlag( unsigned char* databuf ) const
{
    char* buf = nullptr;
    const od_uint32 nullvalue = 0;
    mInsertToCharBuff( databuf, nullvalue, 0, mSizeTwoBytes );
    switch ( complevel_ )
    {
	case NoComp:
	    break;
	case SuperFast:
	    OD::setBitValue( databuf[0], 1, 1 );
	    OD::setBitValue( databuf[0], 2, 1 );
	case Fast:
	    OD::setBitValue( databuf[0], 2, 1 );
	case Normal:
	    OD::setBitValue( databuf[0], 1, 0 );
	    OD::setBitValue( databuf[0], 2, 0 );
	case Maximum:
	    OD::setBitValue( databuf[0], 1, 1 );
	default:
	    OD::setBitValue( databuf[0], 1, 0 );
	    OD::setBitValue( databuf[0], 2, 0 );
    };
}


bool ZipHandler::setLocalFileHeaderForFile( const ZipFileInfo& fileinfo )
{
    unsigned char headerbuff[1024];
    writeGeneralPurposeFlag( headerbuff+mLGenPurBitFlag );
    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, fileinfo.compmethod_,
		       mLCompMethod, mSizeTwoBytes );

    const bool needzip64 = fileinfo.needZIP64( true );
    const od_uint32 uncompsize32bits = needzip64 ? mZIP64SizeLimit
				     : fileinfo.uncompsize_;
    if ( needzip64 )
	mInsertToCharBuff( headerbuff, uncompsize32bits,
			   mLCompSize, mSizeFourBytes)

    mInsertToCharBuff( headerbuff, uncompsize32bits,
		       mLUnCompSize, mSizeFourBytes );

    return setLocalFileHeader( fileinfo, headerbuff );
}


bool ZipHandler::setLocalFileHeaderForDir( const ZipFileInfo& fileinfo )
{
    unsigned char headerbuff[1024];
    for ( int idx=5; idx<mLCompMethod; idx++ )
	headerbuff[idx] = '\0';

    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, fileinfo.compsize_,
		       mLCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, fileinfo.uncompsize_,
		       mLUnCompSize, mSizeFourBytes );

    return setLocalFileHeader( fileinfo, headerbuff );
}


bool ZipHandler::setLocalFileHeaderForLink( const ZipFileInfo& fileinfo )
{
    unsigned char headerbuff[1024];
    for ( int idx=5; idx<mLCompMethod; idx++ )
	headerbuff[idx] = '\0';

    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, fileinfo.compsize_,
		       mLCompSize, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, fileinfo.uncompsize_,
		       mLUnCompSize, mSizeFourBytes );

    return setLocalFileHeader( fileinfo, headerbuff );
}


bool ZipHandler::setLocalFileHeader( const ZipFileInfo& fileinfo,
				     unsigned char* headerbuff )
{
    char* buf = nullptr;

    const od_uint16 version = mVerNeedToExtract;
    mLocalFileHeaderSig ( headerbuff );
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtract, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, fileinfo.compmethod_,
		       mLCompMethod, mSizeTwoBytes );
    const od_uint16 dostime = fileinfo.getDosNrSec();
    mInsertToCharBuff( headerbuff, dostime, mLLastModFTime, mSizeTwoBytes );
    const od_uint16 dosdate = fileinfo.getDosNrDays();
    mInsertToCharBuff( headerbuff, dosdate, mLLastModFDate, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, fileinfo.crc_, mLCRC32, mSizeFourBytes );

    const StringView srcfnm( fileinfo.getFileName() );
    const od_uint16 srcfnmsize = mCast( od_uint16, srcfnm.size() );
    mInsertToCharBuff( headerbuff, srcfnmsize, mLFnmLength, mSizeTwoBytes );
    const od_uint16 xtrafldlength = fileinfo.getHeaderLength( true );
    mInsertToCharBuff( headerbuff, xtrafldlength,
		       mLExtraFldLength, mSizeTwoBytes );

    if ( !ostrm_->addBin(headerbuff,mLHeaderSize) )
	return reportWriteError();

    ostrm_->add( srcfnm );
    if ( !ostrm_->isOK() )
	return reportWriteError();

    if ( !setXtraHeaders(fileinfo,true) )
	return reportWriteError();

    ostrm_->flush();
    return true;
}


bool ZipHandler::setXtraHeaders( const ZipFileInfo& fileinfo, bool local )
{
    bool haserrors = false;
    if ( fileinfo.needZIP64(local) )
	haserrors |= setZIP64Header( fileinfo, local );

    haserrors |= setXtraTimestampFld( fileinfo, local );
    if ( local )
	return haserrors;

    haserrors |= setXtraNTFSFld( fileinfo );
    if ( fileinfo.hasUIDGID() )
	haserrors |= setUnixUIDGID( fileinfo );

    return haserrors;
}


bool ZipHandler::setZIP64Header( const ZipFileInfo& fileinfo, bool local )
{
    const od_uint16 sz = fileinfo.getZIP64Size( local );
    DataBuffer headerbuf( sz, 1 );
    unsigned char* headerbuff = headerbuf.data();
    const od_uint16 headerid = mZIP64Tag;
    const od_uint16 datasize = sz - 2*mSizeTwoBytes;

    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, headerid, mExtraFieldTagPos, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, datasize, mExtraFieldTSizePos,mSizeTwoBytes);

    od_uint64 off = mExtraFieldTSizePos + mSizeTwoBytes;
    if ( local && fileinfo.uncompsize_ > m32BitSizeLimit )
    {
	mInsertToCharBuff( headerbuff,fileinfo.uncompsize_,off,mSizeEightBytes);
	off += mSizeEightBytes;
	mInsertToCharBuff( headerbuff, fileinfo.compsize_, off,mSizeEightBytes);
	off += mSizeEightBytes;
    }
    else if ( !local )
    {
	if ( fileinfo.uncompsize_ > m32BitSizeLimit )
	{
	    mInsertToCharBuff( headerbuff, fileinfo.uncompsize_,
			       off, mSizeEightBytes );
	    off += mSizeEightBytes;
	}

	if ( fileinfo.compsize_ > m32BitSizeLimit )
	{
	    mInsertToCharBuff( headerbuff, fileinfo.compsize_,
			       off, mSizeEightBytes );
	    off += mSizeEightBytes;
	}
    }

    if ( !local )
    {
	if ( fileinfo.localheaderoffset_ > m32BitSizeLimit )
	{
	    mInsertToCharBuff( headerbuff, fileinfo.localheaderoffset_,
			       off, mSizeEightBytes );
	}
    }

    return ostrm_->addBin( headerbuff, sz );
}


bool ZipHandler::setXtraNTFSFld( const ZipFileInfo& fileinfo )
{
    const od_uint16 headerid = mNTFSTag;
    const od_uint16 datasize = mSizeOfNTFSHeader;
    const od_uint32 nullvalue = 0;
    const od_uint16 tag1 = 1;
    const od_uint16 size1 = 3*mSizeEightBytes;

    const int sz = mSizeOfNTFSHeader+2*mSizeTwoBytes;
    DataBuffer headerbuf( sz, 1 );
    unsigned char* headerbuff = headerbuf.data();
    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, headerid, mExtraFieldTagPos, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, datasize, mExtraFieldTSizePos,
		       mSizeTwoBytes );
    od_uint64 off = mExtraFieldTSizePos + mSizeTwoBytes;
    mInsertToCharBuff( headerbuff, nullvalue, off, mSizeFourBytes );
    off += mSizeFourBytes;
    mInsertToCharBuff( headerbuff, tag1, off, mSizeTwoBytes );
    off += mSizeTwoBytes;
    mInsertToCharBuff( headerbuff, size1, off, mSizeTwoBytes );
    off += mSizeTwoBytes;

    const std::timespec modtime = fileinfo.times_.getModificationTime();
    const std::timespec acctime = fileinfo.times_.getAccessTime();
    const std::timespec crtime = fileinfo.times_.getCreationTime();
    const od_uint64 ntfsmodtime = Time::getNTFSFromPosix( modtime );
    const od_uint64 ntfsacctime = Time::getNTFSFromPosix( acctime );
    const od_uint64 ntfscrtime = Time::getNTFSFromPosix( crtime );

    mInsertToCharBuff( headerbuff, ntfsmodtime, off, mSizeEightBytes );
    off += mSizeEightBytes;
    mInsertToCharBuff( headerbuff, ntfsacctime, off, mSizeEightBytes );
    off += mSizeEightBytes;
    mInsertToCharBuff( headerbuff, ntfscrtime, off, mSizeEightBytes );
    off += mSizeEightBytes;

    return ostrm_->addBin( headerbuff, sz );
}


bool ZipHandler::setXtraTimestampFld( const ZipFileInfo& fileinfo, bool local )
{
    const int sz = fileinfo.getTimeStampHeaderLength( local );
    DataBuffer headerbuf( sz, 1 );
    unsigned char* headerbuff = headerbuf.data();
    const od_uint16 headerid = mExtTimeStampTag;
    const od_uint16 datasize = sz - 2*mSizeTwoBytes;
    const char infobit = '\3';

    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, headerid, mExtraFieldTagPos, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, datasize, mExtraFieldTSizePos,
		       mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, infobit, mExtTimeStampFlagPos, mSizeOneByte);
    const std::timespec modtime = fileinfo.times_.getModificationTime();
    mInsertToCharBuff( headerbuff, modtime.tv_sec,
		       mExtTimeStampModTimePos, mSizeFourBytes );
    if ( local )
    {
	const std::timespec acctime = fileinfo.times_.getAccessTime();
	mInsertToCharBuff( headerbuff, acctime.tv_sec,
			   mExtTimeStampAccTimePos, mSizeFourBytes );
    }

    return ostrm_->addBin( headerbuff, sz );
}


bool ZipHandler::setUnixUIDGID( const ZipFileInfo& fileinfo )
{
    unsigned char headerbuff[mSizeOfUIDGIDHeader];
    const od_uint16 headerid = mInfoZipNewUnitTag;
    const od_uint16 datasize = mSizeOfUIDGIDHeader - 2*mSizeTwoBytes;
    static char version = '\1';
    static char uidsz = sizeof(od_uint32);
    static char gidsz = sizeof(od_uint32);

    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, headerid, mExtraFieldTagPos, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, datasize,
		       mExtraFieldTSizePos, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, version, mUIDGIDVerPos, mSizeOneByte );
    mInsertToCharBuff( headerbuff, uidsz, mUIDSizePos, mSizeOneByte );
    mInsertToCharBuff( headerbuff, fileinfo.uid_, mUIDPos, mSizeFourBytes );
    mInsertToCharBuff( headerbuff, gidsz, mGIDSizePos, mSizeOneByte );
    mInsertToCharBuff( headerbuff, fileinfo.gid_, mGIDPos, mSizeFourBytes );

    return ostrm_->addBin( headerbuff, mSizeOfUIDGIDHeader );
}


bool ZipHandler::setEndOfArchiveHeaders(
				    const ObjectSet<ZipFileInfo>& fileinfos )
{
    const od_stream_Pos startchpos = ostrm_->position();
    bool ret = setCentralDirHeader( fileinfos );
    if ( !ret )
	{ closeOutputStream(); return false; }

    const od_stream_Pos endchpos = ostrm_->position();
    const od_uint32 sizecntrldir = endchpos - startchpos;
    const od_int64 totalentries = cumulativefilecounts_.last() +
							    initialfilecount_;
    if ( startchpos > m32BitSizeLimit || totalentries > m16BitDiskLimit )
    {
	if ( !setZIP64EndOfDirRecord(startchpos) ||
	     !setZIP64EndOfDirLocator(endchpos) )
	    { closeOutputStream(); return false; }
    }

    ret = setEndOfCentralDirHeader ( startchpos, sizecntrldir );
    closeOutputStream();
    return ret;
}


bool ZipHandler::setCentralDirHeader( const ObjectSet<ZipFileInfo>& fileinfos )
{
    const BufferString destfile = ostrm_->fileName();
    od_istream deststrm( destfile.buf() );
    if ( !deststrm.isOK() )
	return reportStrmReadError( &deststrm, destfile.buf() );

    unsigned char headerbuff[1024];
    char* buf;
    const od_uint32 nullvalue = 0;
    const od_uint32 zipversion = mVersionZip;
    const od_uint16 os = __iswin__ ? 0 : 3;
    const od_uint16 version = mVerNeedToExtract;

    mCntrlDirHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, zipversion, mLVerMadeBy, mSizeOneByte );
    mInsertToCharBuff( headerbuff, os, mLOSMadeBy, mSizeOneByte );
    mInsertToCharBuff( headerbuff, version, mLVerNeedToExtractCentral,
		       mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNoStart, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLIntFileAttr, mSizeTwoBytes);
    for ( const auto* fileinfo : fileinfos )
    {
	writeGeneralPurposeFlag( headerbuff+mLGenPurBitFlagCentral );
	mInsertToCharBuff( headerbuff, fileinfo->compmethod_,
			   mLCompMethodCentral, mSizeTwoBytes );
	const od_uint16 dostime = fileinfo->getDosNrSec();
	mInsertToCharBuff( headerbuff, dostime, mLastModFTimeCentral,
			   mSizeTwoBytes );
	const od_uint16 dosdate = fileinfo->getDosNrDays();
	mInsertToCharBuff( headerbuff, dosdate, mLastModFDateCentral,
			   mSizeTwoBytes );
	mInsertToCharBuff( headerbuff, fileinfo->crc_, mLCRCCentral,
			   mSizeFourBytes );
	const od_uint32 compsize32bits = fileinfo->compsize_ > m32BitSizeLimit
				       ? mZIP64SizeLimit : fileinfo->compsize_;
	const od_uint32 uncompsize32bits =
				fileinfo->uncompsize_ > m32BitSizeLimit
				? mZIP64SizeLimit : fileinfo->uncompsize_;
	mInsertToCharBuff( headerbuff, compsize32bits, mLCompSizeCentral,
			   mSizeFourBytes );
	mInsertToCharBuff( headerbuff, uncompsize32bits, mLUnCompSizeCentral,
			   mSizeFourBytes );

	const StringView srcfnm( fileinfo->getFileName() );
	const od_uint16 srcfnmsize = mCast( od_uint16, srcfnm.size() );
	mInsertToCharBuff( headerbuff, srcfnmsize, mLFnmLengthCentral,
			   mSizeTwoBytes );

	const od_uint16 xtrafldlength = fileinfo->getHeaderLength( false );
	mInsertToCharBuff( headerbuff, xtrafldlength, mLExtraFldLengthCentral,
			   mSizeTwoBytes );
	const od_uint16 commentsz = mCast(od_uint16, fileinfo->comment_.size());
	mInsertToCharBuff( headerbuff, commentsz, mLFileComntLength,
			   mSizeTwoBytes );
	fileinfo->writeAttrToBuffer( headerbuff+mLExtFileAttr );

	const od_uint32 offset32bit =
			    fileinfo->localheaderoffset_ > m32BitSizeLimit
			    ? mZIP64SizeLimit : fileinfo->localheaderoffset_;
	mInsertToCharBuff( headerbuff, offset32bit, mLRelOffset,mSizeFourBytes);

	if ( !ostrm_->addBin(headerbuff,mCentralHeaderSize) ||
	     !ostrm_->addBin(srcfnm.str(),srcfnmsize) )
	    return reportWriteError();

	if ( !setXtraHeaders(*fileinfo,false) )
	    return reportWriteError();

	if ( commentsz > 0 )
	{
	    if ( !ostrm_->addBin(fileinfo->comment_.str(),commentsz) )
		return reportWriteError();
	}
    }

    return true;
}


bool ZipHandler::setZIP64EndOfDirRecord( od_stream::Pos eodpos )
{
    unsigned char headerbuff[mZIP64EndOfDirRecordSize];
    char* buf;
    mZIP64EndOfDirRecordHeaderSig( headerbuff );

    const od_int64 sizeofheader = mZIP64EndOfDirRecordSize - 12;
    mInsertToCharBuff( headerbuff, sizeofheader, mLSizeOfData, mSizeEightBytes);

    const od_uint32 zipversion = mVersionZip;
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

    const od_stream::Pos ptrlocation = ostrm_->position();
    const od_stream::Pos sizecntrldir = ptrlocation - eodpos;
    mInsertToCharBuff( headerbuff, sizecntrldir, mLSizeCentralDir+28,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, eodpos, mLOffsetCentralDir+32,
		       mSizeEightBytes);

    return ostrm_->addBin( headerbuff, mZIP64EndOfDirRecordSize )
		? true : reportWriteError();
}


bool ZipHandler::setZIP64EndOfDirLocator( od_stream_Pos eodpos )
{
    unsigned char headerbuff[mZIP64EndOfDirLocatorSize];
    char* buf = nullptr;
    const od_uint32 nullvalue = 0;
    const od_uint32 numberofdisks = 1;
    mZIP64EndOfDirLocatorHeaderSig( headerbuff );
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeFourBytes);
    mInsertToCharBuff( headerbuff, eodpos, mLZIP64EndOfDirRecordOffset,
		       mSizeEightBytes);
    mInsertToCharBuff( headerbuff, numberofdisks, mLDiskNo+12, mSizeFourBytes);

    return ostrm_->addBin( headerbuff, mZIP64EndOfDirLocatorSize )
		? true : reportWriteError();
}



bool ZipHandler::setEndOfCentralDirHeader( od_stream_Pos cdirpos,
					   od_uint32 sizecntrldir )
{
    unsigned char headerbuff[100];
    mEndOfCntrlDirHeaderSig( headerbuff );
    const od_uint32 nullvalue = 0;
    char* buf = nullptr;
    mInsertToCharBuff( headerbuff, nullvalue, mLDiskNo, mSizeTwoBytes );
    mInsertToCharBuff( headerbuff, nullvalue, mLCentralDirDiskNo,mSizeTwoBytes);
    if ( cdirpos > m32BitSizeLimit )
	cdirpos = mZIP64SizeLimit;

    cumulativefilecounts_[(cumulativefilecounts_.size()-1)] =
			  cumulativefilecounts_.last() + initialfilecount_;
    od_uint16 cumulativefilecount = cumulativefilecounts_.last();
    if ( cumulativefilecounts_.last() > m16BitDiskLimit )
	cumulativefilecount = mZIP64DiskLimit;

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

    if ( !ostrm_->addBin(headerbuff,mEndOfDirHeaderSize) )
	return reportWriteError();

    ostrm_->flush();
    return true;
}


bool ZipHandler::getArchiveInfo( const char* zipfnm,
				 ObjectSet<ZipFileInfo>& zfileinfo )
{
    if ( !File::exists(zipfnm) )
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( zipfnm );
	return false;
    }

    delete istrm_;
    istrm_ = new od_istream( zipfnm );
    if ( istrm_->isBad() )
	return reportReadError( zipfnm );

    if ( !readEndOfCentralDirHeader() )
	return false;

    const bool ret = readCentralDirHeader( &zfileinfo );
    closeInputStream();
    return ret;
}


bool ZipHandler::initUnZipArchive( const char* zipfnm, const char* basepath,
				   ObjectSet<ZipFileInfo>& fileinfos )
{
    if ( !File::exists(zipfnm) )
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( zipfnm );
	return false;
    }

    if ( !File::isDirectory(basepath) && !File::createDir(basepath) )
    {
	const uiString destdir = ::toUiString( basepath );
	errormsg_ = uiStrings::phrCannotCreateDirectory( destdir );
	return false;
    }

    basepath_.set( basepath );
    delete istrm_;
    istrm_ = new od_istream( zipfnm );
    if ( istrm_->isBad() )
	return reportReadError( zipfnm );

    if ( !readEndOfCentralDirHeader() || !readCentralDirHeader(&fileinfos) )
	{ closeInputStream(); return false; }

    nrdonesize_ = 0;
    totalsize_ = 0;
    for ( const auto* fileinfo : fileinfos )
	totalsize_ += fileinfo->uncompsize_;

    return true;
}


bool ZipHandler::unZipFile( const char* zipfnm, const char* outfnm,
			    const char* basepath, ZipFileInfo* fileinforet )
{
    if ( !File::exists(zipfnm) )
    {
	errormsg_ = uiStrings::phrFileDoesNotExist( zipfnm );
	return false;
    }

    const ZipArchiveInfo zai( zipfnm );
    ZipFileInfo fileinfo;
    const ZipFileInfo* retfileinfo = zai.getInfo( outfnm );
    if ( !retfileinfo )
	{ errormsg_ = zai.errMsg(); return false; }

    const od_stream::Pos offset = retfileinfo->localheaderoffset_;
    if ( offset == -1 || !zai.get(outfnm,fileinfo) )
	{ errormsg_ = zai.errMsg(); return false; }

    delete istrm_;
    istrm_ = new od_istream( zipfnm );
    if ( istrm_->isBad() )
	return reportReadError( zipfnm );

    if ( !readEndOfCentralDirHeader() )
	{ closeInputStream(); return false; }

    istrm_->setReadPosition( offset );
    if ( !File::isDirectory(basepath) && !File::createDir(basepath) )
    {
	const uiString destdir = ::toUiString( basepath );
	errormsg_ = uiStrings::phrCannotCreateDirectory( destdir );
	return false;
    }

    totalsize_ = fileinfo.uncompsize_;
    nrdonesize_ = 0;
    basepath_.set( basepath );
    if ( !extractNextFile(fileinfo) )
	return false;

    if ( curfileidx_ == cumulativefilecounts_.last() )
	return true;

    closeInputStream();
    if ( fileinforet )
	*fileinforet = fileinfo;

    return true;
}


bool ZipHandler::readCentralDirHeader( ObjectSet<ZipFileInfo>* zfileinfo )
{
    if ( offsetofcentraldir_ == 0 && !readEndOfCentralDirHeader() )
	return false;

    istrm_->setReadPosition( offsetofcentraldir_ );
    const BufferString srcfile = istrm_->fileName();
    unsigned char headerbuff[1024];
    bool sigcheck;
    for ( int idx=0; idx<cumulativefilecounts_.last(); idx++ )
    {
	istrm_->getBin( headerbuff, mCentralHeaderSize );
	headerbuff[mCentralHeaderSize] = '\0';
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    closeInputStream();
	    errormsg_ = tr("'%1' is not a valid zip archive").arg(srcfile);
	    return false;
	}

	if ( OD::getBitValue(*(headerbuff+mLCentralDirBitFlag),0) )
	{
	    closeInputStream();
	    errormsg_ = tr("Encrypted file::Not supported");
	    return false;
	}

	const bool fromwin = headerbuff[mLOSMadeBy] == '\0';
	mUnusedVar od_uint16 version =
			*mCast( od_uint16*, headerbuff + mLCentralDirVersion );

	const od_uint16 compmethod =
			*mCast(od_uint16*,headerbuff + mLCentralDirCompMethod );
	if ( compmethod != Z_DEFLATED && compmethod != mZ_NOCOMP )
	{
	    errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	    errormsg_.appendPhrase( tr("Compression method used "
					"is not supported") );
	    return false;
	}

	const od_uint16 bitflag = *mCast( od_uint16*, headerbuff +
						    mLCentralDirBitFlag );
	if ( bitflag > 14 )
	{
	    errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	    errormsg_.appendPhrase( tr("Version of zip format needed "
				       "to unpack is not supported") );
	    return false;
	}

	const od_uint16 hfnmsz
			    = *mCast(od_uint16*,headerbuff+mLFnmLengthCentral);
	BufferString srcfnm( (int)(hfnmsz+1), false );
	istrm_->getBin( srcfnm.getCStr(), hfnmsz );
	srcfnm[hfnmsz] = '\0';

	auto* fileinfo = new ZipFileInfo();
	fileinfo->compmethod_ = compmethod;
	const od_uint16 dostime =
			   *(od_uint16*)(headerbuff + mLastModFTimeCentral);
	const od_uint16 dosnrdays =
			   *(od_uint16*)(headerbuff + mLastModFDateCentral);
	fileinfo->setDosTimeDateModified( dosnrdays, dostime );
	fileinfo->crc_ = *(od_uint32*)(headerbuff + mLCRCCentral);
	fileinfo->compsize_ = *(od_uint32*)(headerbuff + mLCompSizeCentral);
	fileinfo->uncompsize_ = *(od_uint32*)(headerbuff + mLUnCompSizeCentral);
	fileinfo->binary_ = headerbuff[mLIntFileAttr] == '\0';
	fileinfo->setAttr( (const unsigned char*)headerbuff+mLExtFileAttr,
			   fromwin );
	fileinfo->localheaderoffset_ = *(od_uint32*)(headerbuff + mLRelOffset);
	fileinfo->setFileName( srcfnm.buf(), basepath_ );

	const od_uint16 xtrafldlength = *mCast( od_uint16*,
                                          headerbuff + mLExtraFldLengthCentral);
	if ( xtrafldlength > 0 )
	{
	    istrm_->getBin( headerbuff + mCentralHeaderSize, xtrafldlength );
	    readXtraFlds( headerbuff + mCentralHeaderSize, xtrafldlength,
			 *fileinfo );
	}

	const od_uint16 commentsz =
				*(od_uint16*)(headerbuff + mLFileComntLength);
	if ( commentsz > 0 )
	{
	    fileinfo->comment_.setMinBufSize( commentsz+1 );
	    istrm_->getBin( fileinfo->comment_.getCStr(), commentsz );
	    fileinfo->comment_[commentsz] = '\0';
	}

	if ( zfileinfo )
	    zfileinfo->add( fileinfo );
	else
	    delete zfileinfo;
    }

    istrm_->setReadPosition( 0 );
    return true;
}


bool ZipHandler::readEndOfCentralDirHeader()
{
    unsigned char sig[mSizeFourBytes];
    mEndOfCntrlDirHeaderSig( sig );
    od_stream::Pos filepos = istrm_->endPosition();
    if ( filepos == 0 )
    {
	errormsg_ = tr("Zip archive is empty");
	return false;
    }

    filepos -= mEndOfDirHeaderSize;
    istrm_->setReadPosition( filepos );
    unsigned char headerbuff[mEndOfDirHeaderSize];
    istrm_->getBin( headerbuff, mSizeFourBytes );
    headerbuff[mSizeFourBytes] = '\0';
    const od_uint32* ihdrbuff = reinterpret_cast<od_uint32*>(headerbuff);
    const od_uint32* isig = reinterpret_cast<od_uint32*>(sig);
    while ( *ihdrbuff != *isig )
    {
	filepos--;
	if ( filepos <= 0 )
	{
	    errormsg_ = tr("Failed to unzip '%1'").arg( istrm_->fileName() );
	    errormsg_.appendPhrase( tr("Zip archive is corrupt "
				       "(cannot find header signature)") );
	    return false;
	}

	istrm_->setReadPosition( filepos );
	istrm_->getBin( headerbuff, mSizeFourBytes );
    }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		    mEndOfDirHeaderSize-mSizeFourBytes );
    istrm_->setReadPosition( 0 );

    offsetofcentraldir_ = *mCast( od_uint32*, headerbuff+mLOffsetCentralDir );
    od_uint16 cumulativefilecount = *mCast(od_uint16*, headerbuff+mLTotalEntry);
    if ( offsetofcentraldir_ > m32BitSizeLimit ||
	 cumulativefilecount > m16BitDiskLimit )
	return readZIP64EndOfCentralDirLocator();

    cumulativefilecounts_ += cumulativefilecount;
    return true;
}


bool ZipHandler::readZIP64EndOfCentralDirLocator()
{
    unsigned char headerbuff[mZIP64EndOfDirLocatorSize];
    unsigned char sig[mSizeFourBytes];
    mZIP64EndOfDirLocatorHeaderSig( sig );
    istrm_->setReadPosition( 0, od_stream::End );
    od_stream::Pos filepos = istrm_->position();
    if ( filepos == 0 )
    {
	errormsg_ = tr("Zip archive is empty");
	return false;
    }

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
	{
	    errormsg_ = tr("Failed to unzip '%1'").arg( istrm_->fileName() );
	    errormsg_.appendPhrase( tr("Zip archive is corrupt "
				       "(cannot find header signature)") );
	    return false;
	}

	istrm_->setReadPosition( filepos );
	istrm_->getBin( headerbuff, mSizeFourBytes );
    }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		      mZIP64EndOfDirLocatorSize-mSizeFourBytes );
    const unsigned char* cdoffbufptr = headerbuff + mLZIP64EndOfDirRecordOffset;
    offsetofcentraldir_ = *mCast( od_uint64*, cdoffbufptr );
    const unsigned char* totdsksptr =
			 headerbuff + mLZIP64EndOfDirLocatorTotalDisks;
    int totaldisks = *mCast( int*, totdsksptr );
    if ( totaldisks > 1 )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( istrm_->fileName() );
	errormsg_.appendPhrase( tr("Multiple disk spanning of zip archive "
				   "is not supported") );
	return false;
    }

    return readZIP64EndOfCentralDirRecord();
}


bool ZipHandler::readZIP64EndOfCentralDirRecord()
{
    unsigned char headerbuff[mZIP64EndOfDirRecordSize];
    unsigned char sig[mSizeFourBytes];
    mZIP64EndOfDirRecordHeaderSig( sig );
    istrm_->setReadPosition( offsetofcentraldir_ );
    istrm_->getBin( headerbuff, mSizeFourBytes );
    headerbuff[mSizeFourBytes] = 0;
    const od_uint32* ihdrbuff = reinterpret_cast<od_uint32*>(headerbuff);
    const od_uint32* isig = reinterpret_cast<od_uint32*>(sig);
    if ( *ihdrbuff != *isig )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( istrm_->fileName() );
	errormsg_.appendPhrase( tr("Zip archive is corrupt") );
	return false;
    }

    istrm_->getBin( headerbuff+mSizeFourBytes,
		    mZIP64EndOfDirRecordSize-mSizeFourBytes );
    offsetofcentraldir_ = *mCast( od_int64*, headerbuff +
					     mLZIP64CentralDirOffset );
    cumulativefilecounts_ += *mCast( od_int64*, headerbuff +
						mLZIP64CentralDirTotalEntry );
    istrm_->setReadPosition( 0 );
    return true;
}


bool ZipHandler::readXtraFlds( const unsigned char* xtrafld, od_uint16 bufsz,
			       ZipFileInfo& fileinfo )
{
    bool haserror = false; int offs = 0;
    while ( offs < bufsz )
    {
	const od_uint16 extratag = *mCast( od_uint16*, xtrafld + offs );
	offs += mSizeTwoBytes;
	const od_uint16 blocksz = *mCast( od_uint16*, xtrafld + offs );
	offs += mSizeTwoBytes;
	const unsigned char* fielddata = xtrafld + offs;
	if ( extratag == mZIP64Tag )
	    haserror |= !readXtraFldForZIP64( fielddata, blocksz, fileinfo );
	else if ( extratag == mNTFSTag )
	    haserror |= !readNTFSExtrField( fielddata, blocksz, fileinfo );
	else if ( extratag == mExtTimeStampTag )
	    haserror |= !readXtraTimestampFld( fielddata, blocksz, fileinfo );
	else if ( extratag == mInfoZipUnixTag )
	    haserror |= !readXtraField1( fielddata, blocksz, fileinfo );
	else if ( extratag == mInfoZipPrevNewUnixTag ||
		  extratag == mInfoZipNewUnitTag )
	    haserror |= !readXtraUIDGID( fielddata, blocksz, extratag,fileinfo);

	offs += blocksz;
    }

    return haserror;
}


bool ZipHandler::readXtraFldForZIP64( const unsigned char* xtrafld,
				      od_uint16 size, ZipFileInfo& fileinfo )
{
    if ( size < mSizeEightBytes )
	return false;

    od_uint64 off = 0;
    if ( fileinfo.uncompsize_ == mZIP64SizeLimit )
    {
	fileinfo.uncompsize_ = *mCast( od_uint64*, xtrafld+off );
	off += mSizeEightBytes;
    }

    if ( fileinfo.compsize_ == mZIP64SizeLimit )
    {
	fileinfo.compsize_ = *mCast( od_uint64*, xtrafld+off );
	off += mSizeEightBytes;
    }

    if ( fileinfo.localheaderoffset_ == mZIP64SizeLimit )
    {
	fileinfo.localheaderoffset_ = *mCast( od_uint64*, xtrafld+off );
	off += mSizeEightBytes;
    }

    return true;
}


bool ZipHandler::readNTFSExtrField( const unsigned char* xtrafld,
				    od_uint16 size, ZipFileInfo& fileinfo )
{
    if ( size != mSizeOfNTFSHeader )
	return false;

    od_uint64 off = mSizeFourBytes;
    const od_uint16 tag1 = *mCast(od_uint16*,xtrafld+off); off += mSizeTwoBytes;
    const od_uint16 size1 = *mCast(od_uint16*,xtrafld+off);off += mSizeTwoBytes;
    if ( tag1 != 1 || size1 != 3*mSizeEightBytes )
	return false;

    // values are stored in tenth of microseconds (1e-7), i.e. 1=100ns
    const od_uint64 ntfsmtime = *mCast(od_uint64*, xtrafld + off);
    off += mSizeEightBytes;
    const od_uint64 ntfsatime = *mCast(od_uint64*, xtrafld + off);
    off += mSizeEightBytes;

    const std::timespec modtime = Time::getPosixFromNTFS( ntfsmtime );
    const std::timespec acctime = Time::getPosixFromNTFS( ntfsatime );
    fileinfo.times_.setModificationTime( modtime ).setAccessTime( acctime );
    fileinfo.hasutcheader_ = true;

    return true;
}


bool ZipHandler::readXtraTimestampFld( const unsigned char* xtrafld,
				       od_uint16 size, ZipFileInfo& fileinfo )
{
    if ( size < mSizeOneByte )
	return false;

    const char mUnusedVar infobits = xtrafld[0];
    od_int64 off = mSizeOneByte;
    if ( size > off )
    {
	fileinfo.hasutcheader_ = true;
	std::timespec modtime;
	modtime.tv_sec = *mCast(od_uint32*, xtrafld + off);
	modtime.tv_nsec = 0;
	fileinfo.times_.setModificationTime( modtime );
	off += mSizeFourBytes;
    }

    if ( size > off )
    {
	std::timespec acctime;
	acctime.tv_sec = *mCast(od_uint32*, xtrafld + off);
	acctime.tv_nsec = 0;
	fileinfo.times_.setAccessTime( acctime );
	off += mSizeFourBytes;
    }

    return true;
}


bool ZipHandler::readXtraField1( const unsigned char* xtrafld, od_uint16 size,
				 ZipFileInfo& fileinfo )
{
    if ( size < mSizeEightBytes )
	return false;

    od_int64 off = 0;
    fileinfo.hasutcheader_ = true;
    std::timespec modtime, acctime;
    modtime.tv_sec = *mCast(od_uint32*, xtrafld + off);
    modtime.tv_nsec = 0;
    fileinfo.times_.setModificationTime( modtime );
    off += mSizeFourBytes;
    acctime.tv_sec = *mCast(od_uint32*, xtrafld + off);
    acctime.tv_nsec = 0;
    fileinfo.times_.setAccessTime( acctime );
    off += mSizeFourBytes;

    if ( size > off )
    {
	fileinfo.uid_ = *mCast(od_uint16*, xtrafld + off);
	off += mSizeTwoBytes;
    }

    if ( size > off )
    {
	fileinfo.gid_ = *mCast(od_uint16*, xtrafld + off);
	off += mSizeTwoBytes;
    }

    return true;
}


bool ZipHandler::readXtraUIDGID( const unsigned char* xtrafld, od_uint16 size,
				 od_uint16 tag, ZipFileInfo& fileinfo )
{
    if ( size < mSizeFourBytes )
	return false;

    od_uint32& uid = fileinfo.uid_;
    od_uint32& gid = fileinfo.gid_;
    if ( tag == mInfoZipPrevNewUnixTag )
    {
	uid = *mCast( od_uint16*, xtrafld );
	gid = *mCast( od_uint16*, xtrafld+mSizeTwoBytes );
	return true;
    }

    const unsigned char mUnusedVar version = xtrafld[0];
    od_int64 off = mSizeOneByte;
    const od_uint8 uidsz = *mCast(od_uint8*, xtrafld + off);
    off += mSizeOneByte;
    if ( uidsz == mSizeTwoBytes )
	uid = *mCast(od_uint16*, xtrafld + off);
    else if ( uidsz == mSizeFourBytes )
	uid = *mCast(od_uint32*, xtrafld + off);
    else if ( uidsz == mSizeEightBytes )
	uid = mCast( od_uint32, *mCast(od_uint64*, xtrafld + off) );

    off += uidsz;
    const od_uint8 gidsz = *mCast(od_uint8*, xtrafld + off);
    off += mSizeOneByte;
    if ( gidsz == mSizeTwoBytes )
	gid = *mCast(od_uint16*, xtrafld + off);
    else if ( gidsz == mSizeFourBytes )
	gid = *mCast(od_uint32*, xtrafld + off);
    else if ( gidsz == mSizeEightBytes )
	gid = mCast( od_uint32, *mCast(od_uint64*, xtrafld + off) );

    return true;
}


bool ZipHandler::extractNextFile( ZipFileInfo& fileinfo )
{
    allfilenames_.add( fileinfo.getFullFileName() );
    if ( !readLocalFileHeader(fileinfo) )
    {
	closeInputStream();
	return false;
    }

    const StringView destfnm = fileinfo.getFullFileName();
    const char* destfile = destfnm.buf();
    if ( fileinfo.isDirectory() )
    {
	if ( !File::exists(destfile) && !File::createDir(destfile) )
	{
	    closeInputStream();
	    const uiString destdir = ::toUiString( destfile );
	    errormsg_ = uiStrings::phrCannotCreateDirectory( destdir );
	    return false;
	}
    }
    else
    {
	if ( !openStreamToWrite(destfile) )
	    { closeOutputStream(); closeInputStream(); return false; }

	const od_uint16 compmethod = fileinfo.compmethod_;
	const od_uint64 compfilesize = fileinfo.compsize_;
	if ( compfilesize == 0 )
	{}
	else if ( compmethod == Z_DEFLATED && complevel_ != NoComp )
	{
	    if ( !doZUnCompress(fileinfo) )
		{ closeOutputStream(); closeInputStream(); return false; }
	}
	else if ( (compmethod == Z_DEFLATED && complevel_ == NoComp) ||
		   compmethod == mZ_NOCOMP )
	{
	    const od_uint32 chunksize = mMIN( mMaxChunkSize, compfilesize );
	    od_int64 count = chunksize;
	    mAllocLargeVarLenArr( char, in, chunksize );
	    if ( !in )
	    {
		closeInputStream();
		closeOutputStream();
		errormsg_ = uiStrings::phrCannotAllocateMemory( chunksize );
		return false;
	    }

	    for ( bool finish=false; !finish; )
	    {
		const od_stream::Count tohandle = count <= compfilesize
				    ? chunksize : compfilesize % chunksize;
		char* inptr = in.ptr();
		const bool inpfail = !istrm_->getBin( inptr, tohandle);
		const bool outfail = !ostrm_->addBin( inptr, tohandle);
		if ( count > compfilesize )
		    finish = true;

		count += tohandle;
		if ( inpfail || outfail )
		{
		    errormsg_ = tr("Failed to unzip '%1'" )
						.arg( istrm_->fileName() );
		    if ( inpfail )
			istrm_->addErrMsgTo( errormsg_ );
		    else
			ostrm_->addErrMsgTo( errormsg_ );

		    closeInputStream();
		    closeOutputStream();
		    return false;
		}
	    }
	}
	else
	{
	    closeOutputStream();
	    errormsg_ = tr("Failed to unzip '%1'").arg( istrm_->fileName() );
	    errormsg_.appendPhrase( tr("Compression method used "
					"is not supported") );
	    closeInputStream();
	    return false;
	}

	closeOutputStream();
    }

    if ( fileinfo.isSymbolicLink() )
    {
	od_istream deststrm( destfile );
	if ( !deststrm.isOK() )
	{
	    errormsg_ = tr("Failed to restore link '%1'").arg( destfile );
	    return false;
	}

	BufferString linkbuff;
	deststrm.getAll( linkbuff );
	deststrm.close();
	File::remove( destfile );
	BufferString linktarget( destfile );
	if ( __iswin__ )
	{
	    // Restore Unix symbolic links as Windows shortcuts
	    linktarget.add( ".lnk" );
	    FilePath linkfp( destfile );
	    linkfp.setFileName( nullptr ).add( linkbuff );
	    const BufferString linksrc = linkfp.fullPath();
	    File::remove( linktarget );
	    File::createLink( linksrc.buf(), linktarget.buf() );
	}
	else
	    File::createLink( linkbuff, linktarget.buf() );

	if ( fileinfo.hasModTime() )
	    File::setTimes( linktarget.buf(), fileinfo.times_, false );
    }
    else if ( !fileinfo.isDirectory() )
    {
	if ( !fileinfo.getPermissions().isUdf() )
		File::setPermissions( destfile, fileinfo.getPermissions() );

	if ( fileinfo.hasModTime() )
	    File::setTimes( destfile, fileinfo.times_, false );
    }
    // Do NOT set folder permissions before all files are unpacked

    curfileidx_++;
    if ( curfileidx_ == cumulativefilecounts_.last() )
	closeInputStream();

    return true;
}


bool ZipHandler::readLocalFileHeader( ZipFileInfo& fileinfo )
{
    const BufferString srcfile = istrm_->fileName();
    od_uint16& compmethod = fileinfo.compmethod_;
    od_uint64& uncompfilesize = fileinfo.uncompsize_;
    od_uint64& compfilesize = fileinfo.compsize_;
    od_uint32& crc = fileinfo.crc_;

    unsigned char headerbuff[1024];
    istrm_->getBin( headerbuff, mLHeaderSize );
    headerbuff[mLHeaderSize] = '\0';
    if ( istrm_->lastNrBytesRead() != mLHeaderSize )
	return reportReadError();

    bool sigcheck;
    mFileHeaderSigCheck( headerbuff, 0 );
    if ( !sigcheck )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	errormsg_.appendPhrase( tr("Zip archive is corrupt") );
	return false;
    }

    if ( OD::getBitValue(*(headerbuff+mLGenPurBitFlag),0) )
    {
	errormsg_ = tr("Encrypted file: Not supported");
	return false;
    }

    mUnusedVar od_uint16 version =
			*mCast( od_uint16*, headerbuff + mLVerNeedToExtract );

    compmethod = *mCast( od_uint16*, headerbuff + mLCompMethod );
    if ( compmethod != Z_DEFLATED && compmethod != mZ_NOCOMP )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	errormsg_.appendPhrase( tr("Compression method used "
				    "is not supported") );
	return false;
    }

    od_uint16 bitflag = *mCast( od_uint16*, headerbuff + mLGenPurBitFlag );
    if ( bitflag > 14 )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	errormsg_.appendPhrase( tr("Version of zip format needed "
				   "to unpack is not supported") );
	return false;
    }

    crc = *mCast( od_uint32*, headerbuff + mLCRC32 );
    compfilesize = *mCast( od_uint32*, headerbuff + mLCompSize );
    uncompfilesize = *mCast( od_uint32*, headerbuff + mLUnCompSize );
    if ( !fileinfo.hasutcheader_ )
    {
	fileinfo.setDosTimeDateModified(
			    *mCast( od_uint16*, headerbuff + mLLastModFDate ),
			    *mCast( od_uint16*, headerbuff + mLLastModFTime ) );
    }

    const od_uint16 srcfnmsize = *mCast( od_uint16*, headerbuff + mLFnmLength );
    const od_uint16 xtrafldlth = *mCast( od_uint16*,
					 headerbuff + mLExtraFldLength );
    istrm_->getBin( headerbuff, srcfnmsize );
    if ( istrm_->lastNrBytesRead() != srcfnmsize )
	return reportReadError();

    headerbuff[srcfnmsize] = '\0';
    const StringView destfnm( (const char*)(headerbuff) );
    fileinfo.setFileName( destfnm.buf(), basepath_ );
    if ( xtrafldlth > 0 )
    {
	istrm_->getBin( headerbuff, xtrafldlth );
	readXtraFlds( headerbuff, xtrafldlth, fileinfo );
    }

    return true;
}


bool ZipHandler::openStreamToWrite( const char* destfile )
{
    const FilePath destfp( destfile );
    const BufferString pathonly = destfp.pathOnly();
    if ( !File::exists( pathonly.buf() ) )
	File::createDir( pathonly.buf() );

    if ( File::exists(destfile) )
    {
	if ( !File::isWritable(destfile) )
	    File::setWritable( destfile, true );
	if ( File::isSymLink(destfile) && !File::remove(destfile) )
	    return reportWriteError();
    }

    delete ostrm_;
    ostrm_ = new od_ostream( destfile );

    return ostrm_->isOK() ? true : reportWriteError();
}


bool ZipHandler::doZUnCompress( const ZipFileInfo& fileinfo )
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
	errormsg_ = tr("Error Details: Initialization required to "
		       "uncompress data fails");
	errormsg_.appendPhrase( tr("Error code: %1").arg(ret) );
	return false;
    }

    const od_uint64& compfilesize = fileinfo.compsize_;
    const od_uint32 chunksize = mMIN( mMaxChunkSize, compfilesize );
    mDeclareAndTryAlloc( unsigned char*, in, unsigned char[chunksize] );
    mDeclareAndTryAlloc( unsigned char*, out, unsigned char[chunksize] );
    if ( !in || !out )
    {
	delete [] in; delete [] out;
	errormsg_ = uiStrings::phrCannotAllocateMemory( 2*chunksize );
	return false;
    }

    od_int64 count = chunksize;
    od_uint32 crc = 0;
    od_uint32 bytestowrite;
    int flushpolicy = Z_NO_FLUSH;
    const BufferString srcfile = istrm_->fileName();
    do
    {
	const od_stream::Count tohandle = count <= compfilesize
			    ? chunksize : compfilesize % chunksize;
	istrm_->getBin( in, tohandle );
	if ( count > compfilesize )
	    flushpolicy =  Z_FINISH;

	count += tohandle;
	zlibstrm.avail_in = (uInt)istrm_->lastNrBytesRead();
	if ( zlibstrm.avail_in == 0 )
	    break;

	zlibstrm.next_in = mCast( Bytef*, in );
	do
	{
	    zlibstrm.avail_out = chunksize;
	    zlibstrm.next_out = mCast( Bytef*, out );
	    ret = inflate( &zlibstrm, flushpolicy );
	    if ( ret < 0 && ret != Z_BUF_ERROR )
	    {
		delete [] in; delete [] out;
		(void)inflateEnd( &zlibstrm );
		errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
		errormsg_.appendPhrase( tr("Zip archive is corrupt") );
		return false;
	    }

	    bytestowrite = chunksize - zlibstrm.avail_out;
	    nrdonesize_ += bytestowrite;
	    crc = crc32( crc, mCast(Bytef*,out), bytestowrite );
	    if ( !ostrm_->addBin(out,bytestowrite) )
	    {
		delete [] in; delete [] out;
		(void)inflateEnd( &zlibstrm );
		errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
		errormsg_.appendPhrase(
				tr("Error occured while writing to disk") );
		return false;
	    }

	} while ( zlibstrm.avail_out == 0  );

    } while ( flushpolicy != Z_FINISH );

    delete [] in; delete [] out;
    inflateEnd( &zlibstrm );

    if ( !(crc == fileinfo.crc_) )
    {
	errormsg_ = tr("Failed to unzip '%1'").arg( srcfile );
	errormsg_.appendPhrase( tr("Zip archive is corrupt") );
	return false;
    }

    return ret == Z_STREAM_END ? true : false;
#else
    pErrMsg( "ZLib not available" );
    return false;
#endif
}


uiString ZipHandler::errMsg() const
{
    return errormsg_;
}


int ZipHandler::getCumulativeFileCount( int dir ) const
{
    if ( cumulativefilecounts_.validIdx(dir) )
	return cumulativefilecounts_[dir];

    return -1;
}
