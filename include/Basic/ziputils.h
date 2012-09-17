#ifndef ziputils_h
#define ziputils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
 RCS:		$Id: ziputils.h,v 1.4 2012/02/07 08:10:30 cvsranojay Exp $
________________________________________________________________________

-*/
#include "bufstringset.h"

mClass ZipUtils
{
public:
				ZipUtils(const char* filelistnm=0);
    bool			Zip(const char* src,const char* dest);
    bool			UnZip(const char* scr, const char* dest);
    const char*			errorMsg() const	{ return errmsg_.buf();}
    void			makeFileList(const char* zipfile);
    const BufferStringSet&	getFileList() const	{ return filelist_; }

protected:
    bool			doZip(const char* src,const char* dest);
    bool			doUnZip(const char* src,const char* dest);

    BufferString		errmsg_;
    BufferStringSet		filelist_;
    BufferString		filelistname_;
    bool			needfilelist_;
};

#endif
