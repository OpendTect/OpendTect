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

    static const FileSystemAccess&	get(const char* fnm);
			    /*!<Looks at first part of filename to determine
				what system it belongs to. If nothing matches,
				local FileSystemAccess will be returned. */
    static const FileSystemAccess&	getByProtocol(const char* prot);
    static const FileSystemAccess&	getLocal();
    static void		getProtocolNames(BufferStringSet&,bool forread);

    virtual bool	exists(const char*) const;
    virtual bool	isReadable(const char*) const			= 0;
    virtual bool	isFile(const char*) const;
    virtual bool	isDirectory(const char*) const	{ return false; }
    virtual od_int64	getFileSize(const char*,bool followlink) const;
			//!< 0 for non-existing, -1 for unknown
    virtual BufferString timeCreated(const char*) const		{ return ""; }
    virtual BufferString timeLastModified(const char*) const	{ return ""; }

    virtual bool	remove(const char*,bool recursive=true) const
			{ return false; }
    virtual bool	setWritable(const char*,bool yn,
				    bool recursive=true) const
			{ return false; }
    virtual bool	isWritable(const char*) const
			{ return false; }
    virtual bool	rename(const char* from,const char* to,
				uiString* errmsg=nullptr) const
			{ return false; }
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=nullptr) const
			{ return false; }
    virtual bool	createDirectory(const char*) const
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

    const BufferString& errMsg() const			{ return errmsg_; }

protected:

    virtual				~FileSystemAccess() {}

    static const FileSystemAccess&	gtByProt(BufferString&);
    TaskRunner*		getTaskRunner() const;

    mutable BufferString	errmsg_;

private:

    TaskRunner*		getTaskRunner(TaskRunner* newtrun,bool set) const;

public:
    void		setTaskRunner(TaskRunner*) const;

public:
		// Will become virtual
    bool		copy(const char* from,const char* to,
			     uiString* errmsg,bool preserve) const;
    BufferString	timeCreated(const char*,bool followlink) const;
    BufferString	timeLastModified(const char*,bool followlink) const;
    od_int64		getTimeInMilliSeconds(const char*,bool lastmodif,
					      bool followlink) const;
			//!< since epoch (POSIX)
    bool		getTimes(const char*,Time::FileTimeSet&,
				 bool followlink) const;
    bool		setTimes(const char*,const Time::FileTimeSet&,
				 bool followlink) const;
};

} // namespace OD
