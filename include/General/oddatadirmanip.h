#ifndef oddatadirmanip_h
#define oddatadirmanip_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: oddatadirmanip.h,v 1.1 2007-09-19 14:54:09 cvsbert Exp $
________________________________________________________________________

-*/


#include "gendefs.h"

bool OD_isValidRootDataDir(const char*);
const char* OD_SetRootDataDir(const char*);
	    //!< return err msg (or null on success)


#endif
