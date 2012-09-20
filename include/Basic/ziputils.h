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

mClass(Basic) ZipUtils
{
public:
				ZipUtils(const char* filelistnm=0);
				~ZipUtils();
   
    bool			Zip(const char* src,const char* dest);
    bool			UnZip(const char* scr, const char* dest);
    const char*			errorMsg() const{ return errmsg_.buf();}
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }

    bool			unZipArchive(BufferString&,BufferString&,
					     TaskRunner* tr=0); 
    bool			unZipFile(BufferString&,BufferString&);
    bool			makeZip(BufferString&,TaskRunner* tr=0,
	    			   ZipHandler::CompLevel cl=ZipHandler::Normal);
    bool			appendToArchive(BufferString&,BufferString&,
					   TaskRunner* tr=0,
				    ZipHandler::CompLevel c=ZipHandler::Normal);

protected:

    bool			doZip(const char* src,const char* dest);
    bool			doUnZip(const char* src,const char* dest);
    BufferString		errmsg_;
    BufferStringSet		filelist_;
    BufferString		filelistname_;
    bool			needfilelist_ ;

    ZipHandler&			ziphdler_;
};


mClass(Basic) Zipper : public Executor
{
public:
				 Zipper(ZipHandler& zh)
				     : Executor( "Compressing Files" )
				     , ziphd_(zh)
				     , nrdone_(0)	{}

    const char*			 message() const;
    od_int64			 nrDone() const;
    const char*			 nrDoneText() const;
    od_int64			 totalNr() const;

protected:

    int				 nextStep();
    ZipHandler&			 ziphd_;
    int				 nrdone_;
};


mClass(Basic) UnZipper : public Executor
{
public:
				 UnZipper(ZipHandler& zh)
				     : Executor("Uncompressing Files")
				     , ziphd_(zh)
				     , nrdone_(0)	{}

    const char*			 message() const;
    od_int64			 nrDone() const;
    const char*			 nrDoneText() const;
    od_int64			 totalNr() const;

protected:

    int				 nextStep();
    ZipHandler&			 ziphd_;
    int				 nrdone_;
};


#endif

