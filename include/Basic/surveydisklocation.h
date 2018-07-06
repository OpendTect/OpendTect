#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2018
________________________________________________________________________

-*/

#include "bufstring.h"

namespace File { class Path; }


/*!\brief location on disk */

mExpClass(Basic) SurveyDiskLocation
{
public:

			SurveyDiskLocation(); // current
			SurveyDiskLocation(const char* dirnm,const char* bp=0);
			SurveyDiskLocation(const File::Path& fulldir);
    mImplSimpleEqOpers2Memb(SurveyDiskLocation,basepath_,dirname_)

    BufferString	basepath_;	//!< The 'data root'
    BufferString	dirname_;	//!< The subdirectory name

    void		set(const File::Path&);
    BufferString	fullPath() const;
    BufferString	surveyName() const;
    bool		isCurrentSurvey() const;

};
