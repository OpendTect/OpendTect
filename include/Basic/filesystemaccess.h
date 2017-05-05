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

mExpClass(Basic) SystemAccess : public RefCount::Referenced
{
public:

    static
    RefMan<SystemAccess> get(const char* fnm);
			/*!<Looks at first part of filename to determine
			    what system it belongs to. If nothing
			    matches, LocalFileSystemAccess will be returned. */

    virtual bool	exists(const char*,bool forread) const		= 0;
    virtual bool	isReadable(const char*) const			= 0;
    virtual bool	isFile(const char*) const			= 0;
    virtual bool	isDirectory(const char*) const			= 0;

    virtual bool	remove(const char*,bool recursive=true) const	= 0;
    virtual bool	setWritable(const char*,bool yn,
				    bool recursive=true) const		= 0;
    virtual bool	isWritable(const char*) const			= 0;
    virtual bool	rename(const char* from,const char*)		= 0;
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=0) const			= 0;
    virtual od_int64	getFileSize(const char* fnm,bool followlink) const = 0;
    virtual bool	createDirectory(const char*) const		= 0;
    virtual bool	listDirectory(const char*,DirListType,BufferStringSet&,
				      const char* mask) const		= 0;

    virtual StreamData	createOStream(const char*,
				    bool binary=true,
				    bool editmode=false) const	= 0;
			/*!< On win32, binary mode differs from text mode.
			   Use binary=false when explicitly reading
			   txt files.
			   Use editmode=true when want to edit/modify
			   existing data in a file.*/
    virtual StreamData	createIStream(const char*,bool binary=true) const = 0;
			//!< see makeOStream remark

    mDefineFactoryInClass(SystemAccess, factory);

    static BufferString getProtocol(const char* fnm,bool acceptnone);
    static BufferString removeProtocol(const char*);

protected:

    virtual			~SystemAccess() {}

};


mExpClass(Basic) LocalFileSystemAccess : public SystemAccess
{ mODTextTranslationClass(LocalFileSystemAccess);
public:
    virtual bool	exists(const char*,bool forread) const;
    virtual bool	isReadable(const char*) const;
    virtual bool	isFile(const char*) const;
    virtual bool	isDirectory(const char*) const;

    virtual bool	remove(const char*,bool recursive=true) const;
    virtual bool	setWritable(const char*,bool yn,bool recursive) const;
    virtual bool	isWritable(const char*) const;
    virtual bool	rename(const char* from,const char*);
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=0) const;
    virtual od_int64	getFileSize(const char*, bool followlink) const;
    virtual bool	createDirectory(const char*) const;
    virtual bool	listDirectory(const char*,DirListType,BufferStringSet&,
				      const char* mask) const;

    virtual StreamData	createOStream(const char*,
				    bool binary,bool editmode) const;

    virtual StreamData	createIStream(const char*,bool binary) const;

    static void		initClass();
    static const char*	sFactoryKeyword() { return "file"; }
    static uiString	sFactoryDisplayName() { return tr("Local file"); }

    virtual const char* factoryKeyword() const { return sFactoryKeyword(); }
    virtual uiString	factoryDisplayName() const
			{ return sFactoryDisplayName(); }


private:

    static SystemAccess* createInstance() { return new LocalFileSystemAccess; }

};

} // namespace File
