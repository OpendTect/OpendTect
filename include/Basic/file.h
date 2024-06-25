#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "enums.h"
#include "gendefs.h"
#include "integerid.h"
#include "timefun.h"

class BufferStringSet;
class Executor;
class TaskRunner;
namespace Crypto { enum class Algorithm; }


/*!\brief Interface for several file and directory related services */

namespace File
{

enum class Type
{
    File=0,
    Directory=1,
    SymLink=2,
    Alias=3,
    Shortcut=4,
    Character=5,
    Block=6,
    Fifo=7, //named pipe
    Socket=8,
    URI=9,
    Other=9,
    Unknown=10
};

enum class Permission
{
    ReadOwner=0x4000, WriteOwner=0x2000, ExeOwner=0x1000,
    ReadUser=0x0400, WriteUser=0x0200, ExeUser=0x0100,
    ReadGroup=0x0040, WriteGroup=0x0020, ExeGroup=0x0010,
    ReadOther=0x0004, WriteOther=0x0002, ExeOther=0x0001
};

/*!\brief File permissions class, should most often be a combination
	  File::Permission flags on Unix, but uses directly
	  file attributes for local files and directories on Windows,
	  see SetFileAttributes / GetFileAttributes documentation */

mExpClass(Basic) Permissions : public ::IntegerID<od_int32>
{
public:
				Permissions(int perms);
				Permissions(const Permissions&);

    Permissions&		operator =(const Permissions&);

    using IntegerID::IntegerID;
    static inline Permissions	udf()		{ return Permissions(); }

    bool	testFlag(const Permission&) const;
    bool	isReadOnly() const		{ return !isWritable(); }
    bool	isWritable() const;
    bool	isHidden() const;
    bool	isSystem() const;
    int		get_st_mode(const Type&) const;
    bool	isWindowsAttr() const		{ return iswindowsattr_; }


    Permissions& setFlag(const Permission&,bool on);
    Permissions& setReadOnly(bool yn);
    Permissions& setHidden(bool yn);
    Permissions& setSystem(bool yn);

    static Permissions getDefault(bool forfile);
    static Permissions getDefault(bool forfile,bool forwindows);
    static Permissions getFrom(int st_mode,int uid);
    static int	get_st_mode(const char* fnm);

private:

    bool	iswindowsattr_;
    bool	isuid_ = false;
    bool	isgid_ = false;
    bool	sticky_ = false;

public:
			Permissions(int perms,bool iswindowsattr);
			Permissions(int perms,bool isuid,
				    bool isgid,bool sticky);
};

enum class SizeUnit
{
    Bytes=0,
    KB=1,
    MB=2,
    GB=3,
    TB=4,
    PB=5,
    Auto=6
};

mDeclareNameSpaceEnumUtils(Basic,SizeUnit)

mGlobal(Basic) bool		exists(const char*);
				/*!< always follows links, returns false
				     if the link is broken */
mGlobal(Basic) Permissions	getPermissions(const char* fnm);
mGlobal(Basic) bool		isReadable(const char*);
mGlobal(Basic) bool		isWritable(const char*);
mGlobal(Basic) bool		isExecutable(const char*);
mGlobal(Basic) bool		isHidden(const char*);
mGlobal(Basic) bool		isSystem(const char*);
mGlobal(Basic) bool		isLocal(const char*);
mGlobal(Basic) bool		isURI(const char*);
mGlobal(Basic) Type		getType(const char*,bool followlinks=false);
mGlobal(Basic) bool		isFile(const char*);
mGlobal(Basic) bool		isDirectory(const char*);
mGlobal(Basic) bool		isSymLink(const char*);
				/*!< true if the file points to a symbolic link,
				     shortcut (Windows) or alias (macOS) */
mGlobal(Basic) bool		isSymbolicLink(const char*);
				/*!< true if the object points to a
				     symbolic link */
mGlobal(Basic) bool		isShortcut(const char*);
				/*!< true if the object points to a shortcut */
mGlobal(Basic) bool		isInUse(const char* fnm);

mGlobal(Basic) BufferString	findExecutable(const char* exenm,
					       const BufferStringSet& paths,
					       bool includesyspath=true );

mGlobal(Basic) const char*	getCanonicalPath(const char*);
mGlobal(Basic) const char*	getAbsolutePath(const char* dir,
						const char* relfnm);
mGlobal(Basic) const char*	getRelativePath(const char* reltodir,
						const char* fnm);
mGlobal(Basic) void		makeRecursiveFileList(const char* dir,
					BufferStringSet& filelist,
					bool followlinks=false);

mGlobal(Basic) bool		setPermissions(const char*,const Permissions&);
mGlobal(Basic) bool		setReadOnly(const char* fnm,
					    bool recursive=false);
mGlobal(Basic) bool		setWritable(const char*,bool yn,
					    bool recursive=false);
mGlobal(Basic) bool		setExecutable(const char*,bool yn,
					      bool recursive=false);
mGlobal(Basic) bool		setHiddenFileAttrib(const char*,bool yn);
				//!<only Windows local files
mGlobal(Basic) bool		setSystemFileAttrib(const char*,bool yn);
				//!<only Windows local files

enum DirListType		{ AllEntriesInDir, FilesInDir, DirsInDir };
mGlobal(Basic) bool		listDir(const char*,DirListType,
					BufferStringSet& fnames,
					const char* mask=nullptr);
mGlobal(Basic) bool		createDir(const char*);
mGlobal(Basic) bool		createLink(const char* from,const char* to);
				/*!< will be a shortcut on Windows and
				     a symbolic link on Unix */
mGlobal(Basic) bool		rename(const char* oldname,const char* newname,
				       uiString* errmsg=nullptr);
mGlobal(Basic) bool		copy(const char* from,const char* to,
				     bool preserve=true,
				     uiString* errmsg=nullptr,
				     TaskRunner* =nullptr);
mGlobal(Basic) Executor*	getRecursiveCopier(const char* from,
					       const char* to,
					       bool preserve=true);
mGlobal(Basic) Executor*	getRecursiveDeleter(const char* dirnm,
				    const BufferStringSet* externallist=nullptr,
				    bool filesonly=false);
mGlobal(Basic) bool		resize(const char*,od_int64);
mGlobal(Basic) bool		remove(const char*);
mGlobal(Basic) bool		copyDir(const char* from,const char* to,
					bool preserve=true,
					uiString* errmsg=nullptr,
					TaskRunner* =nullptr);
mGlobal(Basic) bool		removeDir(const char*);
				/*!< If path is a link, the target directory
				  is recursively removed, the link remains */
mGlobal(Basic) bool		changeDir(const char* path);
mGlobal(Basic) bool		checkDir(const char* fnm,bool forread,
					 uiString* errmsg=nullptr);
				/*!< checks if the parent directory of a file
				     is readable/writable */

mGlobal(Basic) bool		getContent(const char*,BufferString&);
mGlobal(Basic) BufferString	getHash(const char*,Crypto::Algorithm);
mGlobal(Basic) bool		isEmpty(const char*);
mGlobal(Basic) od_int64		getFileSize(const char* fnm,
					    bool followlink=true);
				// -> returns size in bytes
mGlobal(Basic) BufferString	getFileSizeString(od_int64 filesz,
					File::SizeUnit=File::SizeUnit::Auto);
				// -> takes bytes as input
mGlobal(Basic) BufferString	getFileSizeString(const char* fnm,
					File::SizeUnit=File::SizeUnit::Auto);

mGlobal(Basic) const char*	linkEnd(const char* linkname);
				//!< 'Final' destination of link
				//!< protected against circular links

mGlobal(Basic) const char*	timeCreated(const char* filenm,
					const char* fmt=Time::defDateTimeFmt(),
					bool followlink=true);
mGlobal(Basic) const char*	timeLastModified(const char* filenm,
					const char* fmt=Time::defDateTimeFmt(),
					bool followlink=true);
mGlobal(Basic) od_int64		getTimeInSeconds(const char*filenm,
						 bool modif=true,
						 bool followlink=true);
				//since epoch (POSIX time)
mGlobal(Basic) od_int64		getTimeInMilliSeconds(const char* filenm,
						      bool modif=true,
						      bool followlink=true);
				//since epoch (POSIX time)
mGlobal(Basic) bool		getTimes(const char* filenm,
					 Time::FileTimeSet&,
					 bool followlink=true);
				//!< Retrieve file timestamps
mGlobal(Basic) bool		setTimes(const char* filenm,
					 const Time::FileTimeSet&,
					 bool followlink=true);
				//!< Set file timestamps (if supported by OS)
mGlobal(Basic) bool		waitUntilExists(const char* fnm,
						double maxwaittm,
						double* actualwaited);

mGlobal(Basic) const char*	getCurrentPath();
mGlobal(Basic) const char*	getHomePath();
mGlobal(Basic) const char*	getTempPath();
mGlobal(Basic) const char*	getUserAppDataPath();
mGlobal(Basic) const char*	getRootPath(const char* path);
mGlobal(Basic) int		maxPathLength();

mGlobal(Basic) const char*	asciiFilesFilter();
mGlobal(Basic) const char*	allFilesFilter();
mGlobal(Basic) const char*	textFilesFilter();


enum ViewStyle { Text, Table, Log, Bin };
mDeclareNameSpaceEnumUtils(Basic,ViewStyle)

mClass(Basic) ViewPars
{
public:

				ViewPars( ViewStyle vs=Text )
				    : style_(vs)
				    , editable_(false)
				    , maxnrlines_(vs==Table ? 500 : 10000) {}

    static const char*		sKeyFile()	{ return "file"; }
    static const char*		sKeyMaxLines()	{ return "maxlines"; }
    static const char*		sKeyStyle()	{ return "style"; }
    static const char*		sKeyEdit()	{ return "edit"; }

    ViewStyle			style_;
    int				maxnrlines_;	//!< max nr bytes when Bin
    bool			editable_;	//!< only if possible

};

mGlobal(Basic) bool		launchViewer(const char*,
					const ViewPars& vp=ViewPars());

				/* Internal use only: */
mGlobal(Basic) bool		initTempDir();

// Deprecated functions


mDeprecated("Use linkEnd")
mGlobal(Basic) const char*	linkTarget(const char*);

mDeprecatedDef
mGlobal(Basic) const char*	linkValue(const char* linkname);

mDeprecatedDef
mGlobal(Basic) bool		isDirEmpty(const char*);

mDeprecatedDef
mGlobal(Basic) bool		saveCopy(const char* from,const char* to,
					 bool preserve=true);

mDeprecated("Use File::isSymLink")
mGlobal(Basic) inline bool	isLink( const char* fnm )
				{ return isSymLink( fnm ); }
mDeprecated("Use File::isInUse")
mGlobal(Basic) inline bool	isFileInUse( const char* fnm )
				{ return isInUse( fnm ); }

mDeprecated("Use File::setReadOnly")
mGlobal(Basic) inline bool	makeReadOnly( const char* fnm,bool recursive )
				{ return setReadOnly( fnm, recursive ); }
mDeprecated("Use File::setWritable")
mGlobal(Basic) inline bool	makeWritable( const char* fnm, bool yn,
					      bool recursive )
				{ return setWritable( fnm, yn, recursive ); }
mDeprecated("Use File::setExecutable")
mGlobal(Basic) inline bool	makeExecutable( const char* fnm, bool yn )
				{ return setExecutable( fnm, yn, true ); }
mDeprecated("Use File::setHiddenFileAttrib")
mGlobal(Basic) inline void	hide( const char* fnm, bool yn )
				{ setHiddenFileAttrib( fnm, yn ); }

mDeprecated("Use getFileSize")
mGlobal(Basic) od_int64		getKbSize(const char*);
} // namespace File
