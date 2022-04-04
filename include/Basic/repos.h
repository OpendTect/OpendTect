#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2004
________________________________________________________________________

-*/

#include "basicmod.h"
#include "uistring.h"
#include "bufstring.h"
#include "manobjectset.h"
#include "iopar.h"


namespace Repos
{

    enum Source		{ Temp, Rel, ApplSetup, Data, Survey, User };

/*!\brief Constructs repository file names.

  The basename is the name of a file in upper and lower case,
  e.g. UnitsOfMeasure.

  Then the files to find are:
  Temp: temp stor dir (/tmp on unix), file "UnitsOfMeasure"
  Rel: Software (release) dir, data subdir, file "UnitsOfMeasure"
  ApplSetup: Application setup dir, data subdir, file "UnitsOfMeasure"
  Data: survey data root, file ".unitsofmeasure"
  Survey: survey directory, file ".unitsofmeasure"
  User: home/user dir, .od subdir, file "unitsofmeasure"

  The 'Temp' will not be visited by the 'next' iterator, it's more or less
  added for as an undef or initial value. When specifying reverse, the iterator
  will start at User.

  Usage example:

  Repos::FileProvider rfp( "UnitsOfMeasure" )
  while ( rfp.next() )
      addUnitsFromFile( rfp.fileName(), rfp.source() );
*/

    mGlobal(Basic) bool		isUserDefined(Source);
    mGlobal(Basic) uiString	descriptionOf(Source);


mExpClass(Basic) FileProvider
{
public:

			FileProvider( const char* base_name, bool rev=false )
			: basenm_(base_name)
			, rev_(rev)		{ reset(); }

    bool		next()			{ return next(cursource_,rev_);}
    void		reset()			{ cursource_ = Temp; }
    Source		source() const		{ return cursource_; }
    void		setSource( Source s )	{ cursource_ = s; }
    BufferString	fileName() const	{ return fileName(cursource_); }

    BufferString	fileName(Source) const;
    static bool		next(Source&,bool rev=false);

    bool		removeFile(Source);

protected:

    const BufferString	basenm_;
    Source		cursource_;
    bool		rev_;

    void		getFname(BufferString&,bool) const;

};


/*!\brief IOPar with its Repos Source. */

mExpClass(Basic) IOPar : public ::IOPar
{
public:

		IOPar( Source src )
		    : ::IOPar(""), src_(src)		{}
		IOPar( const ::IOPar& iop, Source src=Data )
		    : ::IOPar(iop), src_(src)		{}

    Source	src_;

};


/*!\brief Set of Repos::IOPar with each a unique name for user recognistion. */

mExpClass(Basic) IOParSet : public ManagedObjectSet<IOPar>
{
public:

			IOParSet(const char* basenm);

    int			find(const char*) const;
    ObjectSet<const IOPar> getEntries(Source) const;

    bool		write(Source) const;
    bool		write(const Source* s=0) const;

    BufferString	fileName(Source) const;

protected:

    const BufferString	basenm_;
    IOParSet&		doAdd(IOPar*) override;

};


}; // namespace Repos

