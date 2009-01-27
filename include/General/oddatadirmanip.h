#ifndef oddatadirmanip_h
#define oddatadirmanip_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: oddatadirmanip.h,v 1.2 2009-01-27 11:44:11 cvsranojay Exp $
________________________________________________________________________

-*/


#include "gendefs.h"

mGlobal bool OD_isValidRootDataDir(const char*);
mGlobal const char* OD_SetRootDataDir(const char*);
	    //!< return err msg (or null on success)


#endif
