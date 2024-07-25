#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "file.h"
#include "factory.h"
#include "strmdata.h"

namespace OD
{

/*!\brief Interface to files and directories, whether local or cloud.

 OpendTect objects will always, one way or the other, use this interface to:
 * Get information about files (and directories)
 * Manipulate files (remove, rename, etc.)
 * Get streams to read or write content

 Utilties like od_stream's and File::exists() will use an instance of this class
 to get their services.

  */

mExpClass(Basic) FileSystemAccess
{
public:
			mOD_DisableCopy(FileSystemAccess)

    static const FileSystemAccess&	get(const char* fnm);
			    /*!<Looks at first part of filename to determine
				what system it belongs to. If nothing matches,
				local FileSystemAccess will be returned. */
    static const FileSystemAccess&	getByProtocol(const char* prot);
    static const FileSystemAccess&	getLocal();
    static void		getProtocolNames(BufferStringSet&,bool forread);

    virtual bool	exists(const char*) const;
    virtual File::Permissions getPermissions(const char*) const
			{ return File::Permissions::udf(); }
    virtual bool	isReadable(const char*) const			= 0;
    virtual bool	isWritable(const char*) const	{ return false; }
    virtual bool	isExecutable(const char*) const { return false; }
    virtual File::Type	getType(const char*,bool followlinks) const	= 0;
    virtual bool	isInUse(const char*) const	{ return false; }
    virtual od_int64	getFileSize(const char*,bool followlink) const;
			//!< 0 for non-existing, -1 for unknown
    virtual BufferString linkEnd(const char* linknm) const
			{ return BufferString::empty(); }
    virtual BufferString timeCreated(const char*,bool followlink) const
			{ return BufferString::empty(); }
    virtual BufferString timeLastModified(const char*,bool followlink) const
			{ return BufferString::empty(); }
    virtual od_int64	getTimeInMilliSeconds(const char*,bool lastmodif,
					  bool followlink) const { return -1; }
			//!< since epoch (POSIX)
    virtual bool	getTimes(const char*,Time::FileTimeSet&,
				 bool followlink) const { return false; }
    virtual bool	setTimes(const char*,const Time::FileTimeSet&,
				 bool followlink) const
			{ return false; }

    virtual bool	setPermissions(const char*,
				       const File::Permissions&) const
			{ return false; }
    virtual bool	setWritable(const char*,bool yn,
				    bool recursive) const
			{ return false; }
    virtual bool	setExecutable(const char*,bool yn,
				      bool recursive) const
			{ return false; }
    virtual bool	setHidden(const char*,bool yn) const
			{ return false; }
    virtual bool	setSystemAttrib(const char*,bool yn) const
			{ return false; }
    virtual bool	rename(const char* from,const char* to,
				uiString* errmsg=nullptr) const
			{ return false; }
    virtual bool	copy(const char* from,const char* to,bool preserve,
			     uiString* errmsg=nullptr,
			     TaskRunner* =nullptr) const
			{ return false; }
    virtual bool	remove(const char*,bool recursive) const
			{ return false; }
    virtual bool	createDirectory(const char*) const
			{ return false; }
    virtual bool	createLink(const char* srcfnm,const char* lnkfnm) const
			{ return false; }
    virtual bool	listDirectory(const char*,File::DirListType,
				      BufferStringSet&,
				      const char* mask) const
			{ return false; }

    virtual StreamData	createIStream(const char*,bool binary=true) const = 0;
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */
    virtual StreamData	createOStream(const char*,bool binary=true,
				      bool inplaceedit=false) const	= 0;
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */

    mDefineFactoryInClass( FileSystemAccess, factory );

    virtual bool	getURIFromURL(const char* url,BufferString& uri,
				      BufferString& region) const
			{ return false; }
    virtual bool	getURLFromURI(const char* uri,const char* region,
				      BufferString& url) const
			{ return false; }

    static BufferString getProtocol(const char* fnm);
    static BufferString withoutProtocol(const char*);
    static BufferString	iconForProtocol(const char*);
    static BufferString	withProtocol(const char* fnm,const char* prot);

    bool		isLocal() const;
    virtual const char*	protocol() const		= 0;
    virtual uiString	userName() const		= 0;
    virtual BufferString iconName() const
			{ return iconForProtocol(protocol()); }

    virtual bool	readingSupported() const	{ return false; }
    virtual bool	writingSupported() const	{ return false; }
    virtual bool	queriesSupported() const	{ return false; }
    virtual bool	operationsSupported() const	{ return false; }

    uiString		errMsg() const			{ return errmsg_; }

protected:
			FileSystemAccess() = default;
    virtual		~FileSystemAccess() {}

    bool		isFile(const char*) const;
    bool		isDirectory(const char*) const;
    bool		isSymLink(const char*) const;

    static const FileSystemAccess&	gtByProt(BufferString&);

    mutable uiString	errmsg_;
};

} // namespace OD
