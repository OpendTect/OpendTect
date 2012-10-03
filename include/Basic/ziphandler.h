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


mClass(Basic) ZipFileInfo
{
    
public:

				ZipFileInfo(const char* fnm, 
					    unsigned int compsize, 
					    unsigned int uncompsize,
					    unsigned int offset)
				: fnm_(fnm)
				, compsize_(compsize) 
				, uncompsize_(uncompsize)
				, localheaderoffset_(offset)	{}

    BufferString		fnm_;
    unsigned int		compsize_;
    unsigned int		uncompsize_;
    unsigned int		localheaderoffset_;

};


mClass(Basic) ZipHandler
{
public:

    friend class		ZipUtils;
    friend class		Zipper;
    friend class		UnZipper;

    enum CompLevel		{ NoComp=0, SuperFast=1, Fast=3, Normal=6,
				  Maximum=9 };

				ZipHandler()
				: totalfilecount_(0)
				, initialfilecount_(0)
				, srcfilesize_(0)
				, destfilesize_(0)
				, srcfnmsize_(0)
				, offsetofcentraldir_(0)
				, sizeofcentraldir_(0)			{}

    const char*			errorMsg()const;

    bool			getArchiveInfo(const char*,
						    ObjectSet<ZipFileInfo>&);

    bool			getBitValue(const unsigned char byte,
							int bitposition)const;
    void			setBitValue(unsigned char& byte, 
					    int bitposition, bool value)const;

protected:

    bool			initUnZipArchive(const char*,const char*);
    bool			unZipFile(const char* srcfnm,const char* fnm,
					  const char* path);

    bool			doZUnCompress();
    bool			readEndOfCentralDirHeader();
    bool			readFileHeader();

    bool			initMakeZip(const char*,const char*);
    bool			initAppend(const char*,const char*);

    bool			manageDir(const char*);
    bool			doZCompress();
    int				openStrmToRead(const char* src); 
    bool			setLocalFileHeader();
    bool			setLocalFileHeaderForDir();
    bool			setCentralDirHeader();
    bool			setEndOfCentralDirHeader(int);

    short			dateInDosFormat(const char*)const;
    short			timeInDosFormat(const char*)const;
    bool			setTimeDateModified();

    const BufferStringSet&	getAllFileNames() { return allfilenames_; }
    std::ostream&		getDestStream()const { return *osd_.ostrm; }
    std::istream&		getSrcStream()const { return *isd_.istrm; }
    unsigned int		getTotalFileCount()const 
				{ return totalfilecount_; }
    void			closeDestStream() { osd_.close(); }
    void			closeSrcStream() { isd_.close(); }
    StreamData			makeOStreamForAppend(const char*)const;
    void			setCompLevel(CompLevel);


    BufferString		errormsg_;
    BufferStringSet		allfilenames_;
    
    BufferString		srcfile_ ;
    unsigned int		srcfilesize_;
    BufferString		srcfnm_;
    unsigned short		srcfnmsize_;

    BufferString		destbasepath_;
    BufferString	    	destfile_;
    unsigned int		destfilesize_ ;
    BufferString		destfnm_;

    unsigned short		compmethod_;
    int				complevel_;
    
    int				nrlevels_;

    unsigned int		initialfilecount_;
    unsigned int		totalfilecount_;
    
    unsigned short		version_;
    unsigned short		lastmodtime_;
    unsigned short		lastmoddate_;
    
    unsigned short		xtrafldlth_ ;
    unsigned short		commentlen_;

    unsigned long		crc_;

    unsigned int		sizeofcentraldir_;
    unsigned int		offsetofcentraldir_;

    StreamData			osd_;
    StreamData			isd_;
    
};


#endif
