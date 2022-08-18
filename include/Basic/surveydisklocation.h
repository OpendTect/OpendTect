#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstring.h"

class BufferStringSet;
class FilePath;
class SurveyInfo;
namespace Survey { class GeometryManager; }


/*!\brief Survey location on disk.

  The survey itself is the only OpendTect database 'object' that has no ID by
  itself. Thus, if you have to work accross surveys, this object has to be the
  ID of a survey. To make things like 'relocate' easier, the path is split in a
  base path (usualy the 'Data Root') and the survey directory name. If any
  of the two is empty, it will be taken from surrent survey.

  When working accross surveys, you can get the corresponding SurveyInfo and
  GeometryManager, cached for you.

 */

mExpClass(Basic) SurveyDiskLocation
{
public:
			SurveyDiskLocation()	{} //!< current
			SurveyDiskLocation(const char* dirnm,const char* bp=0);
			SurveyDiskLocation(const FilePath& fulldir);
			SurveyDiskLocation(const SurveyDiskLocation&);
			~SurveyDiskLocation();

    bool		operator ==(const SurveyDiskLocation&) const;
    bool		operator !=( const SurveyDiskLocation& oth ) const
			{ return !(*this == oth); }
    SurveyDiskLocation&	operator=(const SurveyDiskLocation&);

    BufferString	basePath() const;
    void		setBasePath( const char* bp )	{ basepath_ = bp; }
    BufferString	dirName() const;
    void		setDirName( const char* dn )	{ dirname_ = dn; }

    void		set(const char* fullpath);
    void		set(const FilePath&);
    BufferString	fullPath() const;
    BufferString	surveyName() const;
    BufferString	fullPathFor(const char* fnm) const;

    bool		isCurrentSurvey() const;
    void		setToCurrentSurvey(bool hard=true);

    const SurveyInfo&	surveyInfo() const;
    const Survey::GeometryManager&
			geometryManager() const;

    bool		isEmpty() const;    //!< current survey, soft path
    void		setEmpty();	    //!< current survey, soft path
    bool		exists() const;

    void		fillPar(IOPar&,bool force=false) const;
    bool		usePar(const IOPar&);

    static const SurveyDiskLocation&	currentSurvey();
    static void		listSurveys(BufferStringSet&,const char* basepath=0);
			//!< returns subdirectory names (not full paths)

protected:

    BufferString	basepath_;	//!< The 'data root'
    BufferString	dirname_;	//!< The survey directory name

};
