#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uistring.h"
class uiParent;
class SurveyInfo;
class BufferStringSet;

namespace uiSurvey
{

mGlobal(uiIo) void	getDirectoryNames(BufferStringSet&,bool fullpath,
				const char* dataroot=0,const char* excludenm=0);

mGlobal(uiIo) bool	userIsOKWithPossibleTypeChange(bool for2d);


mGlobal(uiIo) bool	unzipFile(uiParent*,const char* zipfnm=0,
					const char* destdir=0);

mGlobal(uiIo) bool	zipDirectory(uiParent*,const char* survdirnm=0,
					 const char* zipfnm=0);

mGlobal(uiIo) SurveyInfo* copySurvey(uiParent*,const char* survnm,
			    const char* dataroot,const char* sourcesurvdirnm,
			    const char* destpath);
			//!< destination directory name is implicit in survnm


}
