#ifndef ziphandler_h
#define ziphandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		30 August 2012
 RCS:		$Id: ziphandler.h,v 1.2 2012-08-31 06:01:46 cvsraman Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"

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

#define mInsertToCharBuff( buff, T, pnt, len) \
    buf = ( char* ) &T;\
    for( int i = 0; i<len; i++ ) \
    { \
	buff[pnt+i] = *(buf + i); \
    } \

#define mCntrlFileHeaderSigCheck( buff, ptr ) \
    if( buff[ptr] == 80 && buff[ptr + 1] == 75 && \
	    buff[ptr + 2] == 1 && buff[ptr + 3] == 2 ) \
	    sigcheck = true; 

#define mFileHeaderSigCheck( buff, ptr ) \
    sigcheck = false; \
    if( buff[ptr] == 80 && buff[ptr + 1] == 75 && \
	    buff[ptr + 2] == 3 && buff[ptr + 3] == 4 ) \
	    sigcheck = true; \
    else \
	sigcheck = false; \

#define mHeaderSize 30

mClass(Basic) ZipHandler
{
public:
				ZipHandler()				    {}
    bool			doZCompress(std::istream&, std::ostream& );
    bool			doZUnCompress( std::istream&, std::ostream& );
    bool			dirManage( const char*, std::ostream& );
    const char*			errorMsg();
    bool			openStrmToRead( const char* src, 
							std::ostream& dest ); 
    bool			readEndOfCntrlDirHeader( std::istream&  );
    int				readFileHeader( std::istream& );
    bool			setLocalFileHeader( std::ostream& );
    bool			setLocalFileHeaderForDir( std::ostream& dest );
    bool			setCntrlDirHeader( std::ostream& );
    bool			setEndOfCntrlDirHeader(std::ostream&, int );
    bool			getBitValue( const unsigned char byte,
							int bitposition );
    void			setBitValue( unsigned char& byte, 
						int bitposition, bool value );

    short			dateInDosFormat( const char* );
    short			timeInDosFormat( const char* );
    bool			setTimeDateModified( unsigned short&,
						     unsigned short& );
    BufferStringSet		allfiles_;
    unsigned short		compmethod_;
    BufferString		destbasepath_;
    BufferString	    	destfile_ ;
    BufferString		errormsg_;
    BufferString		srcfile_ ;
    unsigned int		srcfilesize_, destfilesize_ ;
    int				nrlevel_;
    unsigned int		totalfiles_;
    unsigned short		version_;
    unsigned short		lastmodtime_;
    unsigned short		lastmoddate_;
    BufferString		destfnm_;
    BufferString		srcfnm_;
    unsigned short		srcfnmsize_, xtrafldlth_ ;
    unsigned long		crc_;
    unsigned int		sizeofcntrldir_, offsetofcntrldir_;

};

#endif
