#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "uitoolsmod.h"
class uiParent;

// Selects input file if not specified, default destination is BaseDataDir
mGlobal(uiTools) bool uiSurvey_UnzipFile(uiParent*,const char* zipfnm=0,
					const char* destdir=0);
// Returns true if zip file successfully unpacked

// Selects output file if not specified, default is current survey
mGlobal(uiTools) bool uiSurvey_ZipDirectory(uiParent*,const char* survdirnm=0,
					 const char* zipfnm=0);
// Returns true if zip file successfully created
