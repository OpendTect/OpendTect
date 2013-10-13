#ifndef odver_h
#define odver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "basicmod.h"
#include "odversion.h"

#include "gendefs.h"

extern "C" {

    mGlobal(Basic) const char* GetFullODVersion();

}

void mGlobal(Basic) GetSpecificODVersion(const char* typ,BufferString&);
/*!< 'typ' can be "doc" or other like vendor name. if null -> platform */

#endif

