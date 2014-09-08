#ifndef odiconfile_h
#define odiconfile_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Sep 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"


/*!\brief Constructs file names for OD icons.

  The identifier that is required is usually the file name without '.png'.
  You can also pass the file name itself, or a full path.

 */

namespace OD
{

mExpClass(General) IconFile : public NamedObject
{
public:

			IconFile(const char* identifier);

    static bool		isPresent(const char* identifier);

    const char*		fullFileName() const	{ return fullpath_; }

protected:

    BufferString	fullpath_;

};


} // namespace OD



#endif
