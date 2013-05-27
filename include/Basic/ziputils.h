
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
#include "basicmod.h"
#include "bufstringset.h"
#include "executor.h"
#include "ziphandler.h"


/*!
\brief Zip Utilities
*/

mExpClass(Basic) ZipUtils
{
public:
				ZipUtils(const char* filelistnm=0);
				~ZipUtils();
   
    bool			Zip(const char* src,const char* dest);
    bool			UnZip(const char* scr, const char* dest);
    const char*			errorMsg() const{ return errmsg_.buf();}
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }

    static bool			unZipArchive(const char* src,const char* dest,
	    				     BufferString& errmsg,
					     TaskRunner* tr=0);
    static bool			unZipArchives(const BufferStringSet& archvs,
					      const char* dest,
	    				      BufferString& errmsg,
					      TaskRunner* tr=0);
    static bool			unZipFile(const char* ziparchive,
					  const char* fnm,const char* path,
					  BufferString& errmsg);

    static bool			makeZip(const char* zipfilenm,
					const BufferStringSet&,
				        BufferString& errmsg,	
					TaskRunner* tr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			makeZip(const char* zipfilenm,
					const char* tozip,
					BufferString& errmsg,
					TaskRunner* tr=0,
					ZipHandler::CompLevel c=
					ZipHandler::Normal);
    static bool			appendToArchive(const char* zipfile,
						const char* toappend,
						BufferString& errmsg,
						TaskRunner* tr=0,
						ZipHandler::CompLevel c=
						ZipHandler::Normal);

protected:

    bool			doZip(const char* src,const char* dest);
    bool			doUnZip(const char* src,const char* dest);

    BufferString		errmsg_;
    BufferStringSet		filelist_;
    BufferString		filelistname_;
    bool			needfilelist_ ;

};


/*!
\brief It is an Executor class which compresses files into zip format but user
should not use it directly instead use ZipUtils::makeZip.
*/

mExpClass(Basic) Zipper : public Executor
{
public:
				Zipper(ZipHandler& zh)
				: Executor( "Making Zip Archive" )
				, ziphd_(zh)
				, nrdone_(0)	    {}

    const char*			message() const;
    od_int64			nrDone() const;
    const char*			nrDoneText() const;
    od_int64			totalNr() const;

protected:

    int				nextStep();
    ZipHandler&			ziphd_;
    int				nrdone_;
};


/*!
\brief It is an Executor class which uncompresses files of zip format but user
should instead use ZipUtils::UnZipArchive() to unzip complete archive or
ZipUtils::UnZipFile() to take one file out of zip archive.
*/

mExpClass(Basic) UnZipper : public Executor
{
public:
				UnZipper(ZipHandler& zh)
				: Executor("Unpacking Files")
				, ziphd_(zh)
				, nrdone_(0)	    {}

    const char*			message() const;
    od_int64			nrDone() const;
    const char*			nrDoneText() const;
    od_int64			totalNr() const;

protected:

    int				nextStep();
    ZipHandler&			ziphd_;
    int				nrdone_;
};


#endif
