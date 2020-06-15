#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
________________________________________________________________________

-*/
#include "generalmod.h"

#include "executor.h"
#include "ziphandler.h"


/*!
\brief Zip Utilities
*/

mExpClass(General) ZipUtils
{ mODTextTranslationClass(zipUtils);
public:
				ZipUtils(const char* filelistnm=0);
				~ZipUtils();
   
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }

    static bool			unZipArchive(const char* src,const char* dest,
					     uiString& errmsg,
					     TaskRunner* tskr=0);
    static bool			unZipArchives(const BufferStringSet& archvs,
					      const char* dest,
					      uiString& errmsg,
					      TaskRunner* tskr=0);
    static bool			unZipFile(const char* ziparchive,
					  const char* fnm,const char* path,
					  uiString& errmsg);

    static bool			makeZip(const char* zipfilenm,
					const BufferStringSet&,
					uiString& errmsg,
					TaskRunner* tskr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			makeZip(const char* zipfilenm,
					const char* tozip,
					uiString& errmsg,
					TaskRunner* tskr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			appendToArchive(const char* zipfile,
						const char* toappend,
						uiString& errmsg,
						TaskRunner* tskr=0,
						ZipHandler::CompLevel c=
						ZipHandler::Normal);

protected:

    BufferStringSet		filelist_;
    BufferString		filelistname_;
    bool			needfilelist_ ;

};


/*!
\brief It is an Executor class which compresses files into zip format but user
should not use it directly instead use ZipUtils::makeZip.
*/

mExpClass(General) Zipper : public Executor
{ mODTextTranslationClass(Zipper)
public:
				Zipper(const char*,const BufferStringSet&, 
                                       ZipHandler::CompLevel);

                                Zipper(const char*,const char*, 
                                       ZipHandler::CompLevel);

    uiString			message() const;
    od_int64			nrDone() const;
    uiString			nrDoneText() const;
    od_int64			totalNr() const;
    bool                        isOk() const { return isok_; }

protected:

    int				nextStep();
    ZipHandler			ziphd_;
    int				nrdone_;
    bool                        isok_;
};


/*!
\brief It is an Executor class which uncompresses files of zip format but user
should instead use ZipUtils::UnZipArchive() to unzip complete archive or
ZipUtils::UnZipFile() to take one file out of zip archive.
*/

mExpClass(General) UnZipper : public Executor
{ mODTextTranslationClass(UnZipper)
public:
				UnZipper(const char*,const char*);

    uiString			message() const;
    od_int64			nrDone() const;
    uiString			nrDoneText() const;
    od_int64			totalNr() const;
    bool                        isOk() const { return isok_; }

protected:

    int				nextStep();
    ZipHandler			ziphd_;
    int				nrdone_;
    bool                        isok_;
};
