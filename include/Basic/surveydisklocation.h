#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2018
________________________________________________________________________

-*/

#include "bufstring.h"

class SurveyInfo;
namespace File { class Path; }


/*!\brief Survey location on disk.

  The survey itself is the only OpendTect database 'object' that has no ID by
  itself. Thus, if you have to work accross surveys, this has to be the ID
  of a survey. To make things like 'relocate' easier, the path is split in a
  base path (usualy the 'Data Root') and the survey directory name. If any
  of the two is empty, it will be taken from surrent survey.

 */

mExpClass(Basic) SurveyDiskLocation
{
public:

			SurveyDiskLocation()	{} //!< current
			SurveyDiskLocation(const char* dirnm,const char* bp=0);
			SurveyDiskLocation(const File::Path& fulldir);
    bool		operator ==(const SurveyDiskLocation&) const;
    bool		operator !=( const SurveyDiskLocation& oth ) const
			{ return !(*this == oth); }

    BufferString	basePath() const;
    void		setBasePath( const char* bp )	{ basepath_ = bp; }
    BufferString	dirName() const;
    void		setDirName( const char* dn )	{ dirname_ = dn; }

    void		set(const char* fullpath);
    void		set(const File::Path&);
    BufferString	fullPath() const;
    BufferString	surveyName() const;
    BufferString	fullPathFor(const char* fnm) const;

    bool		isCurrentSurvey() const;
    void		setToCurrentSurvey(bool hard=true);

    bool		isEmpty() const;    //!< current survey, soft path
    void		setEmpty();	    //!< current survey, soft path

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const SurveyDiskLocation&	currentSurvey();
    const SurveyInfo&	surveyInfo() const;

protected:

    BufferString	basepath_;	//!< The 'data root'
    BufferString	dirname_;	//!< The subdirectory name

};
