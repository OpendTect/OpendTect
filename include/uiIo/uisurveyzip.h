#ifndef uisurveyzip_h
#define uisurveyzip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "commondefs.h"
#include "uiiomod.h"
class uiParent;

// Selects input file if not specified, default destination is BaseDataDir
mGlobal(uiIo) bool uiSurvey_UnzipFile(uiParent*,const char* zipfnm=0,
					const char* destdir=0);
// Returns true if zip file successfully unpacked

// Selects output file if not specified, default is current survey
mGlobal(uiIo) bool uiSurvey_ZipDirectory(uiParent*,const char* survdirnm=0,
					 const char* zipfnm=0);
// Returns true if zip file successfully created

#endif

