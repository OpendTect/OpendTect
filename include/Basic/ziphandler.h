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


class ZipArchiveInfo;

/*!
\brief Zip file information.
*/

mExpClass(Basic) ZipFileInfo
{
    
public:

				ZipFileInfo(const char* fnm, 
				    od_int64 compsize, 
				    od_int64 uncompsize,
				    od_int64 offset)
				: fnm_(fnm)
				, compsize_(compsize) 
				, uncompsize_(uncompsize)
				, localheaderoffset_(offset)	    {}

    BufferString		fnm_;
    od_int64			compsize_;
    od_int64			uncompsize_;
    od_int64			localheaderoffset_;

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
				, uncompfilesize_(0)
				, compfilesize_(0)
				, offsetofcentraldir_(0)
				, curinputidx_(0)
				, curfileidx_(0)
				, totalsize_(0)
				, nrdonesize_(0)		{}

				~ZipHandler();

    const char*			errorMsg()const;

    bool			getArchiveInfo(const char*,
						ObjectSet<ZipFileInfo>&);

    bool			getFileList(const char*,BufferStringSet&);

    od_uint16			dateInDosFormat(const char*) const;
    od_uint16			timeInDosFormat(const char*) const;
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
    bool			readCentralDirHeader(ObjectSet<ZipFileInfo>* 
						     zfileinfo=0);
    bool			readZIP64EndOfCentralDirLocator();
    bool			readZIP64EndOfCentralDirRecord();
    bool			readXtraFldForZIP64(const char*,int);

    bool			initMakeZip(const char*,BufferStringSet);
    bool			initAppend(const char*,const char*);

    bool			compressNextFile();
    bool			doZCompress();
    int				openStrmToRead(const char* src); 
    bool			setLocalFileHeader();
    bool			setLocalFileHeaderForDir();
    bool			setZIP64Header();
    bool			setEndOfArchiveHeaders();
    bool			setCentralDirHeader();
    bool			setEndOfCentralDirHeader(od_int64,od_uint32);
    bool			setZIP64EndOfDirRecord(od_int64);
    bool			setZIP64EndOfDirLocator(od_int64);

    

    const BufferStringSet&	getAllFileNames() { return allfilenames_; }
    od_int64			getCumulativeFileCount() const 
					{ return cumulativefilecounts_.last(); }
    int				getCumulativeFileCount(int) const;
    void			setCompLevel(CompLevel);

    od_int64			getTotalSize()const  { return totalsize_; }
    od_int64			getNrDoneSize()const  { return nrdonesize_; }

    BufferString		errormsg_;
    BufferStringSet		allfilenames_;
    
    BufferString		srcfile_ ;
    od_uint16			srcfnmsize_;

    BufferString		destbasepath_;
    BufferString		destfile_;

    od_int64			compfilesize_;
    od_int64			uncompfilesize_;

    od_uint16			compmethod_;
    CompLevel			complevel_;
    
    int				curnrlevels_;

    int				curinputidx_;
    int				curfileidx_;
    od_int64			initialfilecount_;
    TypeSet<od_int64>		cumulativefilecounts_;
    
    od_uint16			lastmodtime_;
    od_uint16			lastmoddate_;

    od_uint32			crc_;

    od_int64			offsetofcentraldir_;
    od_int64			offsetoflocalheader_;

    od_int64			totalsize_;
    od_int64			nrdonesize_;

    StreamData			osd_;
    StreamData			isd_;
    
};


#endif
