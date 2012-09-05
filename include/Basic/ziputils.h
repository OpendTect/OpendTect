#ifndef ziputils_h
#define ziputils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
 RCS:		$Id: ziputils.h,v 1.10 2012-09-05 07:09:14 cvssalil Exp $
________________________________________________________________________

-*/
#include "basicmod.h"
#include "bufstringset.h"
#include "executor.h"

class ZipHandler;

mClass(Basic) ZipUtils
{
public:
    enum			complevel_ {NoComp = 0, SuperFast = 1, 
					   Fast = 3, Normal = 6,Maximum = 9};

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
							complevel_ cl=Normal);
    bool			appendFile(BufferString&,BufferString&,
							TaskRunner* tr=0,
							complevel_ cl=Normal);

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
				 Zipper(ZipHandler& zh,TaskRunner* tr)
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
				 UnZipper(ZipHandler& zh, TaskRunner* tr)
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

