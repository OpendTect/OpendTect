/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Salil Agarwal
 Date:          30 August 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: ziphandler.cc,v 1.1 2012-08-31 05:32:07 cvssalil Exp $";

#include "ziphandler.h"

bool ZipHandler::dirManage( const char* src, std::ostream& dest )
{
   
    DirList dlist( src, DirList::DirsOnly, 0 );
    DirList flist( src, DirList::FilesOnly, 0 );

    for( int idx = 0; idx < flist.size(); idx++)
    {
	allfiles_.add( flist.fullPath(idx) );
    }
    for( int idx = 0; idx < dlist.size(); idx++ )
    {
	allfiles_.add( dlist.fullPath(idx) );
	dirManage( dlist.fullPath(idx), dest );
    }
    return 1;
}

bool ZipHandler::openStrmToRead( const char* src, std::ostream& dest )
{
   srcfile_ = src;
   if ( File::isDirectory( src ) == true )
   {
       setLocalFileHeaderForDir( dest );
   }
   else
   {
       StreamData isd = StreamProvider( src ).makeIStream( true );
       isd.istrm;
       if ( !isd.usable() ) 
       {
	    errormsg_ = "Input stream not working";
	    return false;
       }
       const bool ret = doZCompress( *isd.istrm, dest );
       isd.close();
       return ret;
   }
}

bool ZipHandler::doZCompress( std::istream& source, std::ostream& dest )
{
    
    unsigned int ptrlocation = dest.tellp();
    setLocalFileHeader( dest );
    int ret, flush = Z_FINISH;
    unsigned towrite;
    z_stream strm;
    unsigned char* in = new unsigned char[srcfilesize_ + 1];	    
    int level = 6;
    int stategy = Z_DEFAULT_STRATEGY;
    destfilesize_ = 0;
    crc_ = 0;
    crc_ = crc32( crc_, 0, 0 );
    //std::cout<<"\n"<<crc_;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2( &strm, level, 8, -15, 8, 0 );
    unsigned int upperbound = deflateBound( &strm, srcfilesize_ );
    unsigned char* out = new unsigned char[upperbound];
    if ( ret != Z_OK ) 
	{
	    errormsg_ = "Error:Deflate() not initialised properly";
	    return false;	
	}
    do
    {
	source.read( (char*)in , srcfilesize_ + 1 );
	strm.avail_in = source.gcount();
	flush = source.eof() ? Z_FINISH : Z_NO_FLUSH;
	crc_ = crc32( crc_, (Bytef*) in, source.gcount() );
	strm.next_in = in;
	do
	{
	    strm.avail_out = upperbound;
	    strm.next_out = out;
	    ret = deflate( &strm, flush );   
	    if ( ret == Z_STREAM_ERROR )
	    {
		errormsg_ = "Error in Deflate()";
		return false;
	    }
	    towrite = upperbound - strm.avail_out;
	    destfilesize_ = destfilesize_ + towrite;
	    if ( towrite > 0 ) dest.write( (const char*)out , towrite );
	    if ( dest.fail() )
	    {
		(void) deflateEnd ( &strm );
		errormsg_ = "Error in Deflate()";
		return false;
	    }
	} while ( strm.avail_out == 0 );
    } while( flush != Z_FINISH );
    (void) deflateEnd ( &strm );
    dest.seekp(ptrlocation + 14);
    dest.write( (const char*) &crc_, sizeof(unsigned long) );
    dest.write( (const char*) &destfilesize_, sizeof(unsigned int) );
    dest.seekp( mHeaderSize + destfilesize_ + srcfnmsize_ + ptrlocation );
    
    delete [] in;
    delete [] out;
    return true;
}

bool ZipHandler::setLocalFileHeader( std::ostream& dest )
{
    short datetime;
    char headerbuff[1024];
    char* buf = 0;
    srcfilesize_ = ( unsigned int ) File::getFileSize( srcfile_ );
    FilePath fnm = srcfile_ ;
    int p = fnm.nrLevels();
    srcfnm_ = "";
    for ( int idx = ( nrlevel_ - 1 ); idx <= (p - 2); idx++ )
    {
	srcfnm_.add( fnm.dir( idx ) );
	srcfnm_ += "/";
    }
    srcfnm_.add( fnm.fileName() );
    srcfnmsize_ = ( unsigned short ) srcfnm_.size();
    mLocalFileHeaderSig ( headerbuff );
    headerbuff[4] = 20;
    headerbuff[5] = 0;
    headerbuff[6] = 0;
    headerbuff[7] = 0;
    headerbuff[8] = 8;
    headerbuff[9] = 0;
    datetime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, 10, 2 );
    datetime = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, 12, 2 );
    mInsertToCharBuff( headerbuff, srcfilesize_, 22, 4 );
    mInsertToCharBuff( headerbuff, srcfnmsize_, 26, 2 );
    headerbuff[28] = 0;
    headerbuff[29] = 0;
    dest.write( (const char*) headerbuff, mHeaderSize );
    dest.write( srcfnm_.buf(), ( srcfnmsize_ ) );
    return true;
}

bool ZipHandler::setLocalFileHeaderForDir( std::ostream& dest )
{
    short datetime;
    char* buf = 0;
    char headerbuff[1024];
    FilePath fnm = srcfile_ ;
    int p = fnm.nrLevels();
    srcfnm_ = "";
    for ( int idx = ( nrlevel_ - 1 ); idx <= (p - 1); idx++ )
    {
	srcfnm_.add( fnm.dir( idx ) );
	srcfnm_ += "/";
    }
    srcfnmsize_ = ( unsigned short ) srcfnm_.size();
    mLocalFileHeaderSig ( headerbuff );
    headerbuff[4] = 10;
    for ( int idx = 5; idx < 26; idx++ )
	headerbuff[idx] = 0;
    datetime = timeInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, 10, 2 );
    datetime = dateInDosFormat( srcfile_ );
    mInsertToCharBuff( headerbuff, datetime, 12, 2 );
    mInsertToCharBuff( headerbuff, srcfnmsize_, 26, 2 );
    headerbuff[28] = 0;
    headerbuff[29] = 0;
    dest.write( (const char*) headerbuff, mHeaderSize );
    dest.write( srcfnm_.buf(), srcfnmsize_ );
    return true;
}

bool ZipHandler::setCntrlDirHeader( std::ostream& dest )
{
    int ptrlocation = dest.tellp();
    char headerbuff[1024];
    StreamData isdest = StreamProvider( destfile_.buf() ).makeIStream( true );
    std::istream& readdest = *isdest.istrm;
    mCntrlDirHeaderSig( headerbuff );
    headerbuff[4] = 0;
    headerbuff[5] = 0;
    
    headerbuff[32] = 0;
    headerbuff[33] = 0;
    headerbuff[34] = 1;
    headerbuff[35] = 0;
    headerbuff[36] = 0;
    headerbuff[37] = 0;
    headerbuff[38] = 0;
    headerbuff[39] = 0;
    headerbuff[40] = 0;
    headerbuff[41] = 0;
    for( int idx = 0; idx < totalfiles_; idx++ )
    {
	unsigned int offset;
	unsigned int compsize;
	unsigned short fnmsize;
	char localheader[1024];
	char* buf;
	offset = readdest.tellg();
	readdest.read( ( char* )localheader, mHeaderSize);
	localheader[mHeaderSize] = 0;
	compsize = *(unsigned int*)( localheader + 18 );
	fnmsize = *(unsigned short*)( localheader + 26 );
	readdest.read( ( char* )(localheader + mHeaderSize), fnmsize );
	localheader[mHeaderSize + fnmsize] = 0;
	for( int id = 4; id < 30; id++ )
	    headerbuff[id + 2] = localheader[id];
	for( int id = 0; id < fnmsize; id++ )
	    headerbuff[id + 46] = localheader[id + 30];
	mInsertToCharBuff( headerbuff, offset, 42, 4 );
	readdest.seekg( mHeaderSize + compsize + fnmsize + offset );
	dest.write( ( char* )headerbuff, (46 + fnmsize)  );
	/*pnt = readdest.tellg();*/
    } 
    mCntrlDirDigitalSig( headerbuff, 0 );
    headerbuff[4] = 0;
    headerbuff[5] = 0;
    dest.write( ( char* )headerbuff, 6  );
    isdest.close();
    return true;
}

bool ZipHandler::setEndOfCntrlDirHeader( std::ostream& dest, int ptrlctn )
{
    int ptrlocation = dest.tellp();
    int sizecntrldir = ptrlocation - ptrlctn;
    char headerbuff[100];
    char* buf;
    mEndOfCntrlDirHeaderSig( headerbuff );
    headerbuff[4] = 0;
    headerbuff[5] = 0;
    headerbuff[6] = 0;
    headerbuff[7] = 0;
    mInsertToCharBuff( headerbuff, totalfiles_, 8, 2 );
    mInsertToCharBuff( headerbuff, totalfiles_, 10, 2 );
    mInsertToCharBuff( headerbuff, sizecntrldir, 12, 4 );
    mInsertToCharBuff( headerbuff, ptrlctn, 16, 4 );
    headerbuff[20] = 0;
    headerbuff[21] = 0;
    dest.write( (char*) headerbuff, 22 );
    return true;
}

bool ZipHandler::readEndOfCntrlDirHeader( std::istream& src )
{
    char headerbuff[20];
    int ptrlocation;
    char sig[5];
    mEndOfCntrlDirHeaderSig( sig );
    src.seekg( 0 , std::ios::end );
    ptrlocation = src.tellg();
    src.seekg( ptrlocation - 22 , std::ios::beg );
    ptrlocation = src.tellg();
    src.read( (char*) headerbuff, 4 );
    headerbuff[4] = 0;
    while (!( *( unsigned int* )( headerbuff ) == *( unsigned int* )( sig )))
    {
	src.seekg( ( ptrlocation - 1 ) , std::ios::beg );
	ptrlocation = src.tellg();
	src.read( (char*) headerbuff, 4 );
    }
    src.read( (char*) headerbuff, 18 );
    headerbuff[18] = 0;
    src.seekg(0);   

    totalfiles_ = *( ( short* ) ( headerbuff + 6 ) );
    sizeofcntrldir_  = *( ( int* ) ( headerbuff + 8 ) );
    offsetofcntrldir_ = *( ( int* ) ( headerbuff + 12 ) );
    return true;
}

int ZipHandler::readFileHeader( std::istream& src )
{
    char headerbuff[1024];
    int ptrlocation = src.tellg();
    bool sigcheck;
    src.read( (char*) headerbuff, 30 );
    headerbuff[30] = 0;
    if ( src.gcount() != 30 )
    {
	errormsg_ = "Error: Reading operation failed";
	return false;
    }
    headerbuff[30] = 0;
    mFileHeaderSigCheck( headerbuff, 0 );
    if ( !sigcheck )
    {
	errormsg_ = "Local File Header Signature not match";
	return 0;
    }
    if ( getBitValue( *(headerbuff + 6), 1 ) )
    {
	errormsg_ = "Encrypted file::Not supported";
	//TODO
	return 0;
    }
    version_ = *( (unsigned short*)( headerbuff + 4 ) );
    if ( version_ > 20 )
    {
	errormsg_ = "Version needed to extract not supported";
	return 0;
    }
    compmethod_ = *( (unsigned short*)( headerbuff + 8 ) );
    lastmodtime_ = *( (unsigned short*)( headerbuff + 10 ) );
    lastmoddate_ = *( (unsigned short*)( headerbuff + 12 ) );
    crc_ = *( (unsigned int*)( headerbuff + 14 ) );
    srcfilesize_ = *( (unsigned int*)( headerbuff + 18 ) );
    destfilesize_ = *( (unsigned int*)( headerbuff + 22 ) );
    srcfnmsize_ = *( (unsigned short*)( headerbuff + 26 ) );
    xtrafldlth_ = *( (unsigned short*)( headerbuff + 28 ) );
    src.read( (char*) headerbuff, srcfnmsize_ );
    if ( src.gcount() != srcfnmsize_ )
    {
	errormsg_ = "Error: Reading operation failed";
	return 0;
    }
    headerbuff[srcfnmsize_] = 0;
    if ( headerbuff[srcfnmsize_ - 1] == '/' )
    {
	headerbuff[srcfnmsize_ - 1] = 0;
	destfile_ = destbasepath_;
	destfile_ += headerbuff;
	if ( !File::exists( destfile_.buf() ) )
	    File::createDir( destfile_.buf() );
	src.seekg( ptrlocation + srcfnmsize_ + xtrafldlth_ + 30 );
	return -1;
    }

    destfile_ = destbasepath_;
    destfile_ += headerbuff;
    src.seekg( ptrlocation + srcfnmsize_ + xtrafldlth_ + 30 );
    return 1;
}


bool ZipHandler::doZUnCompress( std::istream& source, std::ostream& dest )
{
	int ret, flush;
        unsigned have;
        z_stream strm;
        unsigned char* in = new unsigned char[srcfilesize_];
        unsigned char* out = new unsigned char[destfilesize_];
	unsigned int crc = 0;
	crc = crc32( crc, 0, 0 );
        /* allocate inflate state */
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
	ret = -3;
        ret = inflateInit2( &strm, -15 );
        if ( ret!=Z_OK )
        {
	    errormsg_ = "Error:Inflate() not initialised properly";
	    return false;
        }
        do
        {
            source.read( (char *)in, srcfilesize_ );
            strm.avail_in = source.gcount();
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
		    errormsg_ = "Error in Inflate()";
                    return false;
                }
                have = destfilesize_ - strm.avail_out;
		crc = crc32( crc, (Bytef*) out, have );
		dest.write( (char*)out, have );
            } while ( strm.avail_out == 0 );
            
            /* done when inflate() says it's done */
        } while ( flush != Z_FINISH );
        
	if ( !(crc == crc_) )
	{
	    errormsg_ = "Error:Loss of data possible. CRC not matched";
	    return false;
	}
        /* clean up and return */
        (void)inflateEnd( &strm );

	delete [] in;
	delete [] out;
        return ret == Z_STREAM_END ? true : false;
}

bool ZipHandler::getBitValue(const unsigned char byte, int bitposition)
{
    unsigned char modfbyte;
    modfbyte = byte >> ( bitposition - 1 );
    if ( modfbyte % 2 == 0) return false;
    else return true;
}

void ZipHandler::setBitValue(unsigned char& byte,
						int bitposition, bool value)
{
    unsigned char var = (unsigned char)  pow( 2.0 , (bitposition-1) );
    if ( value ) 
	byte = byte | var;
    else 
    {
	var = ~ var;
	byte = byte & var;
    }
}

const char* ZipHandler::errorMsg()
{
    return errormsg_.buf();
}

short ZipHandler::timeInDosFormat( const char* fnm )
{
    unsigned short dosformat;
    unsigned char bte[2];
    int idx;
    QFileInfo qfi( fnm );
    QTime ftime = qfi.lastModified().time();
    int sec = ftime.second();
    char min = ( char ) ftime.minute();
    char hr = (char) ftime.hour();
    sec = sec/2;
    bte[0] = (char) sec;
    bte[1] = 0;
    for ( idx = 6; idx < 9; idx++ )
	setBitValue( bte[0], idx, getBitValue( min, idx - 5 ) );
    for ( idx = 1; idx < 4; idx++ )
	setBitValue( bte[1], idx, getBitValue( min, idx + 3 ) );
    for ( idx = 4; idx < 9; idx++ )
	setBitValue( bte[1], idx, getBitValue( hr, idx-3 ) );
    dosformat = *( ( unsigned short* ) ( bte ) );
    return dosformat;
}

short ZipHandler::dateInDosFormat( const char* fnm )
{
    unsigned short dosformat;
    unsigned char bte[2];
    int idx;
    QFileInfo qfi( fnm );
    QDate fdate = qfi.lastModified().date();
    unsigned char day = ( char ) fdate.day();
    unsigned char month = ( char ) fdate.month();
    int year = fdate.year();
    unsigned char dosyear;
    dosyear = ( unsigned char ) (year - 1980);
    bte[0] =  day;
    bte[1] = 0;
    for ( int idx = 6; idx < 9; idx++ )
	setBitValue( bte[0], idx, getBitValue( month, idx - 5 ) );
    for ( int idx = 1; idx < 2; idx++ )
	setBitValue( bte[1], idx, getBitValue( month, idx + 3 ) );
    for ( int idx = 2; idx < 9; idx++ )
	setBitValue( bte[1], idx, getBitValue( dosyear, idx - 1 ) );
    dosformat = *( ( unsigned short* ) ( bte ) );
    return dosformat;
}

bool ZipHandler::setTimeDateModified( unsigned short& time,
				      unsigned short& date )
{
    od_int64 timeinsec;
    unsigned char bytetime[2], bytedate[2], byte = 0;
    int sec, min, hour, day, month, year, idx;
    bytetime[0] = *( (char*) ( &time ) );
    bytetime[1] = *( ( (char*) ( &time ) ) + 1 );
    bytedate[0] = *( (char*) ( &date ) );
    bytedate[1] = *( ( (char*) ( &date ) ) + 1 );


    for ( idx = 1; idx < 6; idx++ )
	setBitValue( byte, idx, getBitValue( bytetime[0], idx ) );
    byte = byte * 2;
    sec = byte;
    byte = 0;
    for ( idx = 1; idx < 4; idx++ )
	setBitValue( byte, idx, getBitValue( bytetime[0], idx + 5 ) );
    for ( idx = 4; idx < 7; idx++ )
	setBitValue( byte, idx, getBitValue( bytetime[1], idx - 3 ) );
    min = byte;
    byte = 0;
    for ( idx = 1; idx < 6; idx++ )
	setBitValue( byte, idx, getBitValue( bytetime[1], idx + 3 ) );
    hour = byte;
    byte = 0;


    for ( idx = 1; idx < 6; idx++ )
	setBitValue( byte, idx, getBitValue( bytedate[0], idx ) );
    day = byte;
    byte = 0;
    for ( idx = 1; idx < 4; idx++ )
	setBitValue( byte, idx, getBitValue( bytedate[0], idx + 5 ) );
    for ( idx = 4; idx < 5; idx++ )
	setBitValue( byte, idx, getBitValue( bytedate[1], idx - 3 ) );
    month = byte;
    byte = 0;
    for ( idx = 1; idx < 8; idx++ )
	setBitValue( byte, idx, getBitValue( bytedate[1], idx + 1 ) );
    year = byte + 1980;


    QTime qt( hour, min, sec );
    QDate qd( year, month, day );
    QDateTime qdt( qd, qt );
    timeinsec = qdt.toTime_t();
    if ( __iswin__ )
    {
	struct _utimbuf ut;
	ut.modtime = timeinsec;
	ut.actime = timeinsec;
	if ( _utime( destfile_.buf(), &ut) == -1 )
	    return 0;
    }
    else
    {
	struct utimbuf ut;
	ut.modtime = timeinsec;
	if ( utime( destfile_.buf(), &ut) == -1 )
	    return 0;
    }

    return 1;
}