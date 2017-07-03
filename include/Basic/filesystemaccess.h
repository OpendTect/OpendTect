#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		Jan 2017
________________________________________________________________________

-*/

#include "file.h"


namespace File
{

/*!\brief Interface to files and directories, whether local or cloud.

 OpendTect objects will always, one way or the other, use this interface to:
 * Get information about files (and directories)
 * Manipulate files (remove, rename, etc.)
 * Get streams to read or write content

 Utilties like od_stream's and File::exists() will use an instance of this class
 to get their services.

  */

mExpClass(Basic) SystemAccess : public RefCount::Referenced
{
public:

    typedef RefMan<SystemAccess>    Ref;

    static Ref		get(const char* fnm);
			    /*!<Looks at first part of filename to determine
				what system it belongs to. If nothing matches,
				LocalFileSystemAccess will be returned. */

    virtual bool	exists(const char*) const;
    virtual bool	isReadable(const char*) const			= 0;
    virtual bool	isFile(const char*) const;
    virtual bool	isDirectory(const char*) const;
    virtual od_int64	getFileSize(const char*,bool followlink) const;
			//!< 0 for non-existing, -1 for unknown

    virtual bool	remove(const char*,bool recursive=true) const;
    virtual bool	setWritable(const char*,bool yn,
				    bool recursive=true) const;
    virtual bool	isWritable(const char*) const;
    virtual bool	rename(const char* from,const char*) const;
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=0) const;
    virtual bool	createDirectory(const char*) const;
    virtual bool	listDirectory(const char*,DirListType,BufferStringSet&,
				      const char* mask) const;

    virtual StreamData	createIStream(const char*,bool binary=true) const = 0;
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */
    virtual StreamData	createOStream(const char*,bool binary=true,
				      bool inplaceedit=false) const	= 0;
			/*!< keep binary==true also for text files unless you
			     know what you are doing. win32 thing only. */

    mDefineFactoryInClass(SystemAccess, factory);

    static BufferString getProtocol(const char* fnm,bool acceptnone);
    static BufferString withoutProtocol(const char*);

protected:

    virtual		~SystemAccess() {}

};


} // namespace File
