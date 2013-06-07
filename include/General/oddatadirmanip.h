#ifndef oddatadirmanip_h
#define oddatadirmanip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id$
________________________________________________________________________

-*/


#include "gendefs.h"

mGlobal bool OD_isValidRootDataDir(const char*);
mGlobal const char* OD_SetRootDataDir(const char*);
	    //!< return err msg (or null on success)


#endif
