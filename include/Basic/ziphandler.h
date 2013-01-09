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
    for( od_int32 idx=0; idx<len; idx++ ) \
    { \
	inserttobuffer[insertat+idx] = *(buf + idx); \
    } \

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
	sigcheck = false; \

#define mHeaderSize 30
#define mCentralHeaderSize 46
#define mDigSigSize 6
#define mEndOfDirHeaderSize 22


/*!
\ingroup Basic
\brief Zip file information.
*/

mClass(Basic) ZipFileInfo
{
    
public:

				ZipFileInfo(const char* fnm, 
				    od_uint32 compsize, 
				    od_uint32 uncompsize,
				    od_uint32 offset)
				: fnm_(fnm)
				, compsize_(compsize) 
				, uncompsize_(uncompsize)
				, localheaderoffset_(offset)	{}

    BufferString		fnm_;
    od_uint32			compsize_;
    od_uint32			uncompsize_;
    od_uint32			localheaderoffset_;

};


/*!
\ingroup Basic
\brief Handles zipping and unzipping of files.
*/

mClass(Basic) ZipHandler
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
				, destfilesize_(0)
				, srcfnmsize_(0)
				, offsetofcentraldir_(0)
				, sizeofcentraldir_(0)	{}

    const char*			errorMsg()const;

    bool			getArchiveInfo(const char*,
						ObjectSet<ZipFileInfo>&);

    bool			getBitValue(const unsigned char byte,
						od_int32 bitposition)const;
    void			setBitValue(unsigned char& byte, od_int32 
						bitposition, bool value)const;

	void			setNrLevel(od_int32 level){ nrlevels_ = level; }

protected:

    bool			initUnZipArchive(const char*,const char*);
    bool			unZipFile(const char* srcfnm,const char* fnm,
															const char* path);

    bool			doZUnCompress();
    bool			readEndOfCentralDirHeader();
    bool			readFileHeader();

    bool			initMakeZip(const char*,BufferStringSet);
    bool			initAppend(const char*,const char*);

    bool			manageDir(const char*);
    bool			doZCompress();
    od_int32			openStrmToRead(const char* src); 
    bool			setLocalFileHeader();
    bool			setLocalFileHeaderForDir();
    bool			setCentralDirHeader();
    bool			setEndOfCentralDirHeader(od_int32);

    od_int16			dateInDosFormat(const char*)const;
    od_int16			timeInDosFormat(const char*)const;
    bool			setTimeDateModified();

    const BufferStringSet&	getAllFileNames() { return allfilenames_; }
    std::ostream&		getDestStream()const { return *osd_.ostrm; }
    std::istream&		getSrcStream()const { return *isd_.istrm; }
    od_uint32			getCumulativeFileCount()const 
					{ return cumulativefilecount_.last(); }
    od_int32			getCumulativeFileCount(od_int32)const;
    void			closeDestStream() { osd_.close(); }
    void			closeSrcStream() { isd_.close(); }
    StreamData			makeOStreamForAppend(const char*)const;
    void			setCompLevel(CompLevel);


    BufferString		errormsg_;
    BufferStringSet		allfilenames_;
    
    BufferString		srcfile_ ;
    od_uint32			srcfilesize_;
    BufferString		srcfnm_;
    od_uint16			srcfnmsize_;

    BufferString		destbasepath_;
    BufferString		destfile_;
    od_uint32			destfilesize_ ;
    BufferString		destfnm_;

    od_uint16			compmethod_;
    od_int32			complevel_;
    
    od_int32			nrlevels_;

    od_uint32			initialfilecount_;
    TypeSet<od_uint32>		cumulativefilecount_;
    
    od_uint16			version_;
    od_uint16			lastmodtime_;
    od_uint16			lastmoddate_;
    
    od_uint16			xtrafldlth_ ;
    od_uint16			commentlen_;

    od_uint32			crc_;

    od_uint32			sizeofcentraldir_;
    od_uint32			offsetofcentraldir_;

    StreamData			osd_;
    StreamData			isd_;
    
};


#endif
