#ifndef oddatadirmanip_h
#define oddatadirmanip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: oddatadirmanip.h,v 1.4 2012-08-03 13:00:24 cvskris Exp $
________________________________________________________________________

-*/


#include "generalmod.h"
#include "gendefs.h"

mGlobal(General) bool OD_isValidRootDataDir(const char*);
mGlobal(General) const char* OD_SetRootDataDir(const char*);
	    //!< return err msg (or null on success)


#endif

