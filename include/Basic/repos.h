#ifndef repos_h
#define repos_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: repos.h,v 1.2 2007-01-10 19:01:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

namespace Repos
{

    enum Source	{ Temp, Appl, Data, Survey, User };

/*!\brief constructs repository file names.
 
  The basename is the name of a file in upper and lower case,
  e.g. UnitsOfMeasure.
  
  Then the files to find are:
  Temp: temp stor dir (/tmp on unix) "UnitsOfMeasure"
  Appl: Software dir, data subdir "UnitsOfMeasure"
  Data: survey data root ".unitsofmeasure"
  Survey: survey directory ".unitsofmeasure"
  User: home/user dir, .od subdir "unitsofmeasure"

  The 'Temp' will not be visited by the 'next' iterator, it's more or less
  added for as an undef or initial value.

  Usage example:

  Repos::FileProvider rfp( "UnitsOfMeasure" )
  while ( rfp.next() )
      addUnitsFromFile( rfp.fileName(), rfp.source() );

 */

class FileProvider
{
public:

			FileProvider( const char* base_name )
			: basenm_(base_name)	{ reset(); }

    bool		next()			{ return next(cursource_); }
    void		reset()			{ cursource_ = Temp; }
    Source		source() const		{ return cursource_; }
    BufferString	fileName() const	{ return fileName(cursource_); }

    BufferString	fileName(Source) const;
    bool		next(Source&);

protected:

    Source		cursource_;
    const BufferString	basenm_;

    void		getFname(BufferString&,bool) const;

};


}; // namespace Repos

#endif
