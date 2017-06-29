#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "networkmod.h"

#include "filesystemaccess.h"



/*!\brief provides streams based on web services */
mExpClass(Network) HttpFileAccess : public File::SystemAccess
{ mODTextTranslationClass(HttpFileAccess);
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
    virtual od_int64	getFileSize(const char*,bool followlink) const;

    virtual bool	createDirectory(const char*) const;
    virtual bool	listDirectory(const char*,File::DirListType,
				      BufferStringSet&,const char* mask) const;

    virtual StreamData	createOStream(const char*,
				      bool binary,bool editmode) const;
    virtual StreamData	createIStream(const char*,bool binary) const;

    static void		initClass();
    static const char*	sFactoryKeyword()	{ return "http"; }
    static uiString	sFactoryDisplayName()	{ return tr("Web file"); }

    virtual const char* factoryKeyword() const	{ return sFactoryKeyword(); }
    virtual uiString	factoryDisplayName() const
			{ return sFactoryDisplayName(); }

private:

    static SystemAccess* createInstance() { return new HttpFileAccess; }

};
