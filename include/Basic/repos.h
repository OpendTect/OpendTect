#ifndef repos_h
#define repos_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: repos.h,v 1.5 2008-12-18 05:23:26 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

namespace Repos
{

    enum Source	{ Temp, Rel, ApplSetup, Data, Survey, User };

/*!\brief constructs repository file names.
 
  The basename is the name of a file in upper and lower case,
  e.g. UnitsOfMeasure.
  
  Then the files to find are:
  Temp: temp stor dir (/tmp on unix), file "UnitsOfMeasure"
  Rel: Software (release) dir, data subdir, file "UnitsOfMeasure"
  Appl: Application setup dir, data subdir, file "UnitsOfMeasure"
  Data: survey data root, file ".unitsofmeasure"
  Survey: survey directory, file ".unitsofmeasure"
  User: home/user dir, .od subdir, file "unitsofmeasure"

  The 'Temp' will not be visited by the 'next' iterator, it's more or less
  added for as an undef or initial value.

  Usage example:

  Repos::FileProvider rfp( "UnitsOfMeasure" )
  while ( rfp.next() )
      addUnitsFromFile( rfp.fileName(), rfp.source() );

 */

mClass FileProvider
{
public:

			FileProvider( const char* base_name )
			: basenm_(base_name)	{ reset(); }

    bool		next()			{ return next(cursource_); }
    void		reset()			{ cursource_ = Temp; }
    Source		source() const		{ return cursource_; }
    BufferString	fileName() const	{ return fileName(cursource_); }

    BufferString	fileName(Source) const;
    static bool		next(Source&);

protected:

    Source		cursource_;
    const BufferString	basenm_;

    void		getFname(BufferString&,bool) const;

};


}; // namespace Repos

#endif
