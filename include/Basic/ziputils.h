#ifndef ziputils_h
#define ziputils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
 RCS:		$Id: ziputils.h,v 1.6 2012-08-31 05:32:54 cvssalil Exp $
________________________________________________________________________

-*/
#include "basicmod.h"
#include "bufstringset.h"
# include"ziphandler.h"
# include"ziparchiveinfo.h"



mClass(Basic) ZipUtils
{
public:
				ZipUtils(const char* filelistnm=0);
    bool			Zip(const char* src,const char* dest);
    bool			UnZip(const char* scr, const char* dest);
    const char*			errorMsg() const{ return errmsg_.buf();}
    const char*			errMsg();
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }
    bool			zUnCompress( BufferString&, TaskRunner* ); 
    bool			zUnCompress( BufferString&, BufferString&,
								TaskRunner* );
    //bool			zUnCompress( BufferString&, int, TaskRunner* );
    bool			zCompress( BufferString&, TaskRunner* );
    ZipHandler			ziphdler_;

protected:
    bool			doZip(const char* src,const char* dest);
    bool			doUnZip(const char* src,const char* dest);
    BufferString		errmsg_;
    BufferStringSet		filelist_;
    BufferString		filelistname_;
    bool			needfilelist_ ;
    
};

mClass(Basic) Zipping : public Executor
{
public:
				 Zipping ( BufferStringSet& list, 
					   std::ostream& dest,
					   ZipHandler& zh,
					   TaskRunner* tr )
				     :Executor( "Compressing Files" )
				     , flist_(list)
				     , dest_(dest)
				     , ziphd_(zh)
				     , nrdone_(0)	{}
protected:
    int				 nextStep();
    od_int64			 nrDone() const;
    const char*			 nrDoneText() const;
    od_int64			 totalNr() const;
    std::ostream&		 dest_;
    BufferStringSet		 flist_;
    ZipHandler&			 ziphd_;
    int				 nrdone_;
};

mClass(Basic) UnZipping : public Executor
{
public:
				 UnZipping ( std::istream& src,
					    ZipHandler& zh, TaskRunner* tr )
				     :Executor( "Uncompressing Files" )
				     , src_(src)
				     , ziphd_(zh)
				     , nrdone_(0)	{}
protected:
    const char*			 message() const;
    int				 nextStep();
    od_int64			 nrDone() const;
    const char*			 nrDoneText() const;
    od_int64			 totalNr() const;
    std::istream&		 src_;
    ZipHandler&			 ziphd_;
    int				 nrdone_;
};



#endif

