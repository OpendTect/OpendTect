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

    virtual const char*	protocol() const		= 0;
    virtual uiString	userName() const		= 0;
    virtual BufferString iconName() const
			{ return iconForProtocol(protocol()); }

    virtual bool	readingSupported() const	{ return true; }
    virtual bool	writingSupported() const	{ return true; }
    virtual bool	queriesSupported() const	{ return true; }
    virtual bool	operationsSupported() const	{ return true; }

    const BufferString& errMsg() const			{ return errmsg_; }

protected:
					FileSystemAccess() = default;
    virtual				~FileSystemAccess() {}
    static const FileSystemAccess&	gtByProt(BufferString&);

    mutable BufferString	errmsg_;
};

} // namespace OD
