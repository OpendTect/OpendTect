
#ifndef ziputils_h
#define ziputils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
 RCS:		$Id$
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
				ZipUtils(const char* filelistnm=0);
				~ZipUtils();
   
    bool			Zip(const char* src,const char* dest);
    bool			UnZip(const char* scr, const char* dest);
    uiString			errorMsg() const{ return errmsg_;}
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }

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

protected:

    bool			doZip(const char* src,const char* dest);
    bool			doUnZip(const char* src,const char* dest);
   
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
{
public:
				Zipper(const char*,const BufferStringSet&, 
                                       ZipHandler::CompLevel);

                                Zipper(const char*,const char*, 
                                       ZipHandler::CompLevel);

    uiStringCopy			uiMessage() const;
    od_int64			nrDone() const;
    uiStringCopy			uiNrDoneText() const;
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
{
public:
				UnZipper(const char*,const char*);

    uiStringCopy			uiMessage() const;
    od_int64			nrDone() const;
    uiStringCopy			uiNrDoneText() const;
    od_int64			totalNr() const;
    bool                        isOk() const { return isok_; }

protected:

    int				nextStep();
    ZipHandler			ziphd_;
    int				nrdone_;
    bool                        isok_;
};


#endif
