#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstringset.h"
#include "filepath.h"
#include "od_iosfwd.h"
#include "typeset.h"
#include "uistring.h"

namespace File { class Permissions; enum class Type; }
namespace Time { class FileTimeSet; }

/*!
\brief Zip file information.
*/

mExpClass(General) ZipFileInfo
{

public:
				ZipFileInfo();
				ZipFileInfo(const ZipFileInfo&);
    virtual			~ZipFileInfo();

    ZipFileInfo&		operator =(const ZipFileInfo&);
    bool			operator >(const ZipFileInfo&) const;

    const char*			getFileName() const;
    const char*			getFullFileName() const;
    const char*			getLinkValue() const;
    bool			madeOnWindows() const	{ return sourceiswin_; }
    bool			isDirectory() const;
    bool			isSymbolicLink() const;
    bool			isWritable() const;
    bool			isHidden() const;
    bool			isSystem() const;
    const File::Permissions&	getPermissions() const	{ return perms_; }

    od_uint16			getHeaderLength(bool local) const;
    od_uint16			getZIP64Size(bool local) const;
    bool			hasModTime() const;
    bool			hasUIDGID() const;
    bool			needZIP64(bool local) const;
    int				getTimeStampHeaderLength(bool local) const;
    od_uint16			getDosNrDays() const;
    od_uint16			getDosNrSec() const;
    void			writeAttrToBuffer(unsigned char* buf) const;

    void			setFileName(const char*,
					    const FilePath& basepath);
    void			setFullFileName(const char*,
						const FilePath& basepath);
    void			setAttr(const unsigned char* buf,
					bool fromwin);
    void			setDosTimeDateModified(od_uint16 dosnrdays,
						       od_uint16 dosnrsec);

    od_uint16			compmethod_		= mUdf(od_uint16);
    Time::FileTimeSet&		times_;
    od_uint32			crc_			= mUdf(od_uint32);
    od_uint64			compsize_		= mUdf(od_uint64);
    od_uint64			uncompsize_		= mUdf(od_uint64);
    od_stream_Pos		localheaderoffset_	= mUdf(od_stream_Pos);
    od_uint32			uid_			= mUdf(od_uint32);
    od_uint32			gid_			= mUdf(od_uint32);
    bool			binary_			= true;
    BufferString		comment_;
    bool			hasutcheader_		= false;

private:
    void			setFileNameInZip(const FilePath& basepath);

    BufferString		fullfnm_;
    BufferString		filenminzip_;
    BufferString		linkvalue_;
    File::Type&			type_;
    File::Permissions&		perms_;
    bool			sourceiswin_;
    int				nrlevels_; //cached

};


/*!
\brief Handles zipping and unzipping of files.
*/

mExpClass(General) ZipHandler
{ mODTextTranslationClass(ZipHandler)
public:

    enum CompLevel		{ NoComp=0, SuperFast=1, Fast=3,
				  Normal=6, Maximum=9 };

				ZipHandler();
				~ZipHandler();

    uiString			errMsg()const;

    bool			getArchiveInfo(const char*,
					       ObjectSet<ZipFileInfo>&);

private:

    bool			getFileList(const char*,BufferStringSet&);
    bool			initUnZipArchive(const char* zipnm,
						 const char* dest,
						 ObjectSet<ZipFileInfo>&);
    bool			unZipFile(const char* srcfnm,const char* fnm,
					  const char* path,
					  ZipFileInfo* fileinforet =nullptr);

    bool			extractNextFile(ZipFileInfo&);
    bool			openStreamToWrite(const char* fnm);
    bool			readLocalFileHeader(ZipFileInfo& localinfo);
    bool			readEndOfCentralDirHeader();
    bool			readCentralDirHeader(ObjectSet<ZipFileInfo>*
						     =nullptr);
    bool			readZIP64EndOfCentralDirLocator();
    bool			readZIP64EndOfCentralDirRecord();
    static bool			readXtraFlds(const unsigned char* buf,
					     od_uint16 bufsz,ZipFileInfo&);
    static bool			readXtraFldForZIP64(const unsigned char*,
						    od_uint16,ZipFileInfo&);
    static bool			readNTFSExtrField(const unsigned char*,
						  od_uint16,ZipFileInfo&);
    static bool			readXtraTimestampFld(const unsigned char*,
						     od_uint16,ZipFileInfo&);
    static bool			readXtraField1(const unsigned char*,od_uint16,
					       ZipFileInfo&);
    static bool			readXtraUIDGID(const unsigned char*,
					       od_uint16 sz,od_uint16 tag,
					       ZipFileInfo&);
    bool			doZUnCompress(const ZipFileInfo&);

    bool			initMakeZip(const BufferStringSet& fnms,
					    const char* basepath,
					    const char* outfnm);
    bool			initAppend(const char* srcfnm,
					   const char* basepath,
					   const char* outfnm);

    int				openStrmToRead(const char* src);
    bool			compressNextFile(ZipFileInfo&);
    bool			doZCompress(ZipFileInfo&);
    BufferString		getZipLocalFileName(const char* fnm) const;
    void			writeGeneralPurposeFlag(
						  unsigned char* buf) const;
    bool			setLocalFileHeaderForFile(const ZipFileInfo&);
    bool			setLocalFileHeaderForDir(const ZipFileInfo&);
    bool			setLocalFileHeaderForLink(const ZipFileInfo&);
    bool			setLocalFileHeader(const ZipFileInfo&,
						   unsigned char* headerbuff);
    bool			setXtraHeaders(const ZipFileInfo&,bool local);
    bool			setZIP64Header(const ZipFileInfo&,bool local);
    bool			setXtraNTFSFld(const ZipFileInfo&);
    bool			setXtraTimestampFld(const ZipFileInfo&,
						    bool local);
    bool			setUnixUIDGID(const ZipFileInfo&);
    bool			setEndOfArchiveHeaders(
					const ObjectSet<ZipFileInfo>&);
    bool			setCentralDirHeader(
					const ObjectSet<ZipFileInfo>&);
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

    bool			reportReadError(
					const char* filenm=nullptr) const;
    bool			reportWriteError(
					const char* filenm=nullptr) const;
    bool			reportStrmReadError(od_istream*,
						    const char*) const;
    void			closeInputStream();
    void			closeOutputStream();

    mutable uiString		errormsg_;
    BufferStringSet		allfilenames_;

    CompLevel			complevel_		= Normal;

    int				curinputidx_		= 0;
    int				curfileidx_		= 0;
    od_int64			initialfilecount_	= 0;
    TypeSet<od_int64>		cumulativefilecounts_;
    od_int64			totalsize_		= mUdf(od_int64);
    od_int64			nrdonesize_		= mUdf(od_int64);

    FilePath			basepath_;
    od_stream_Pos		offsetofcentraldir_	= 0;

    od_ostream*			ostrm_			= nullptr;
    od_istream*			istrm_			= nullptr;

    friend class		ZipUtils;
    friend class		Zipper;
    friend class		UnZipper;
    friend class		MultiArchiveUnZipper;

};
