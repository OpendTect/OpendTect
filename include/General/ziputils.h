#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
________________________________________________________________________

-*/
#include "generalmod.h"
#include "bufstringset.h"
#include "executor.h"
#include "ziphandler.h"


/*!
\brief Zip Utilities
*/

mExpClass(General) ZipUtils
{ mODTextTranslationClass(zipUtils);
public:

    static bool			makeFileList(const char* zipfilenm,
					     BufferStringSet& list,
					     uiString& errmsg);

    static bool			unZipArchive(const char* src,const char* dest,
					     uiString& errmsg,
					     TaskRunner* tr=0);
    static bool			unZipArchives(const BufferStringSet& archvs,
					      const char* dest,
					      uiString& errmsg,
					      TaskRunner* tr=0);
    static bool			unZipFile(const char* ziparchive,
					  const char* fnm,const char* path,
					  BufferString& errmsg);

    static bool			makeZip(const char* zipfilenm,
					const BufferStringSet&,
					uiString& errmsg,
					TaskRunner* tr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			makeZip(const char* zipfilenm,
					const char* tozip,
					uiString& errmsg,
					TaskRunner* tr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			appendToArchive(const char* zipfile,
						const char* toappend,
						uiString& errmsg,
						TaskRunner* tr=0,
						ZipHandler::CompLevel c=
						ZipHandler::Normal);

#define mMyDeprecated \
mDeprecated("Use the static functions, see header file for details")

    mMyDeprecated		ZipUtils(const char* filelistnm=0);
				~ZipUtils();
   
    mMyDeprecated bool		Zip(const char* src,const char* dest);
    mMyDeprecated bool		UnZip(const char* scr, const char* dest);
    mMyDeprecated uiString	errorMsg() const{ return errmsg_;}

    mMyDeprecated void			 makeFileList(const char* zipfile);
    mMyDeprecated const BufferStringSet& getFileList() const
					 { return filelist_; }

protected:

    uiString			errmsg_;
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

    uiString			uiMessage() const override;
    od_int64			nrDone() const override;
    uiString			uiNrDoneText() const override;
    od_int64			totalNr() const override;
    bool			isOk() const { return isok_; }

protected:

    int				nextStep() override;
    ZipHandler			ziphd_;
    int				nrdone_;
    bool			isok_;
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

    uiString			uiMessage() const override;
    od_int64			nrDone() const override;
    uiString			uiNrDoneText() const override;
    od_int64			totalNr() const override;
    bool			isOk() const { return isok_; }

protected:

    int				nextStep() override;
    ZipHandler			ziphd_;
    int				nrdone_;
    bool			isok_;
};


