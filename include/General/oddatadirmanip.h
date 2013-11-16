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


/* DEPRECATED! Don't use! Will be gone in 5.0! */


#include "generalmod.h"
#include "gendefs.h"

mGlobal(General) bool OD_isValidRootDataDir(const char*);
	// Replaced by IOMan::isValidDataRoot
mGlobal(General) const char* OD_SetRootDataDir(const char*);
	    //!< Don't do this!


#endif

