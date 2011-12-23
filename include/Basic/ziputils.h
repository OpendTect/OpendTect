#ifndef ziputils_h
#define ziputils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		December  2011
 RCS:		$Id: ziputils.h,v 1.1 2011-12-23 09:55:23 cvsranojay Exp $
________________________________________________________________________

-*/
#include "bufstring.h"

mClass ZipUtils
{
public:
    bool		    Zip(const char* src,const char* dest);
    bool		    UnZip(const char* scr, const char* dest);
    const char*		    errorMsg()const { return errmsg_.buf(); }

protected:
    bool		    doZip(const char* src,const char* dest);
    bool		    doUnZip(const char* src,const char* dest);

    BufferString	    errmsg_;
};

#endif
