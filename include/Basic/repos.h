#ifndef repos_h
#define repos_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: repos.h,v 1.1 2004-11-25 17:23:03 bert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

namespace Repos
{

    enum Source	{ Temp, Survey, Data, User, Appl };

/*!\brief constructs repository file names.
 
  The basename is the name of a file in upper and lower case,
  e.g. UnitsOfMeasure. Then the files to find are:
  Survey: survey directory ".unitsofmeasure"
  Data: survey data root ".unitsofmeasure"
  User: home/user dir, .od subdir "unitsofmeasure"
  Appl: Software dir, data subdir "UnitsOfMeasure"
  Temp: temp stor dir (/tmp on unix) "UnitsOfMeasure"

  The 'Temp' will not be visited by the 'next' iterator, it's more or less
  added for as an undef or initial value.

 */

class FileProvider
{
public:

		FileProvider( const char* base_name )
		: basenm_(base_name)	{ reset(); }

    bool	next()			{ return next(cursource_); }
    void	reset()			{ cursource_ = Survey; }
    Source	source() const		{ return cursource_; }
    BufferString fileName() const	{ return fileName(cursource_); }

    bool	next(Source&);
    BufferString fileName(Source) const;

protected:

    Source		cursource_;
    BufferString	basenm_;

    void		getFname(BufferString&,bool) const;

};


}; // namespace Repos

#endif
