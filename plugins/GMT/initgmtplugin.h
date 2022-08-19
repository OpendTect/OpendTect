#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
