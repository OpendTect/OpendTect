#ifndef ziphandler_h
#define ziphandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		30 August 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "strmprov.h"
#include "typeset.h"

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
#define mDigSigSize 6
#define mEndOfDirHeaderSize 22

class ZipArchiveInfo;

/*!
\brief Zip file information.
*/

mExpClass(Basic) ZipFileInfo
{
    
public:

				ZipFileInfo(const char* fnm, 
				    od_uint32 compsize, 
				    od_uint32 uncompsize,
				    od_uint32 offset)
				: fnm_(fnm)
				, compsize_(compsize) 
				, uncompsize_(uncompsize)
				, localheaderoffset_(offset)	    {}

    BufferString		fnm_;
    od_uint32			compsize_;
    od_uint32			uncompsize_;
    od_uint32			localheaderoffset_;

};


/*!
\brief Handles zipping and unzipping of files.
*/

mExpClass(Basic) ZipHandler
{
public:

    friend class		ZipUtils;
    friend class		Zipper;
    friend class		UnZipper;

    enum CompLevel		{ NoComp=0, SuperFast=1, Fast=3, Normal=6,
								Maximum=9 };

				ZipHandler()
				: initialfilecount_(0)
				, srcfilesize_(0)
				, offsetofcentraldir_(0)
				, curinputidx_(0)
				, curfileidx_(0)
				, ziparchinfo_(0)		{}

				~ZipHandler();

    const char*			errorMsg()const;

    bool			getArchiveInfo(const char*,
						ObjectSet<ZipFileInfo>&);

    bool			getFileList(const char*,BufferStringSet&) const;

    od_int16			dateInDosFormat(const char*) const;
    od_int16			timeInDosFormat(const char*) const;
    bool			setTimeDateModified(const char*,od_uint16,
						    od_uint16)const;

    bool			getBitValue(const unsigned char byte,
							int bitposition) const;
    void			setBitValue(unsigned char& byte, int 
						bitposition, bool value) const;


protected:

    bool			initUnZipArchive(const char*,const char*);
    bool			unZipFile(const char* srcfnm,const char* fnm,
					  const char* path);

    bool			extractNextFile();
    int				readLocalFileHeader();
    bool			openStreamToWrite();

    bool			doZUnCompress();
    bool			readEndOfCentralDirHeader();

    bool			initMakeZip(const char*,BufferStringSet);
    bool			initAppend(const char*,const char*);

    bool			compressNextFile();
    bool			doZCompress();
    int				openStrmToRead(const char* src); 
    bool			setLocalFileHeader();
    bool			setLocalFileHeaderForDir();
    bool			setEndOfArchiveHeaders();
    bool			setCentralDirHeader();
    bool			setEndOfCentralDirHeader(int);

    

    const BufferStringSet&	getAllFileNames() { return allfilenames_; }
    int				getCumulativeFileCount() const 
					{ return cumulativefilecounts_.last(); }
    int				getCumulativeFileCount(int) const;
    StreamData			makeOStreamForAppend(const char*) const;
    void			setCompLevel(CompLevel);


    BufferString		errormsg_;
    BufferStringSet		allfilenames_;
    
    BufferString		srcfile_ ;
    od_uint32			srcfilesize_;
    od_uint16			srcfnmsize_;

    BufferString		destbasepath_;
    BufferString		destfile_;

    od_uint16			compmethod_;
    CompLevel			complevel_;
    
    int				curnrlevels_;

    int				curinputidx_;
    int				curfileidx_;
    int				initialfilecount_;
    TypeSet<int>		cumulativefilecounts_;
    
    od_uint16			lastmodtime_;
    od_uint16			lastmoddate_;

    od_uint32			crc_;

    od_uint32			offsetofcentraldir_;

    ZipArchiveInfo*		ziparchinfo_;

    StreamData			osd_;
    StreamData			isd_;
    
};


#endif
