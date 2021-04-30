#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2012
________________________________________________________________________

*/
#include "gmtmod.h"
#include "gendefs.h"

namespace GMT
{
    mGlobal(GMT) void initStdClasses();


    mGlobal(GMT) bool hasLegacyGMT();
		// Version 4 or below

    mGlobal(GMT) bool hasModernGMT();
		// Version 5 or above

    mGlobal(GMT) bool hasGMT();

    mGlobal(GMT) const char* versionStr();

    mGlobal(GMT) const char* sKeyDefaultExec();

}
