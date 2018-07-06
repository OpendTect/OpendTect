#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2018
________________________________________________________________________

-*/

#include "dbkey.h"
#include "surveydisklocation.h"


/*!\brief DBKey valid in multi-survey contexts */

mExpClass(Basic) FullDBKey : public DBKey
{
public:

			    FullDBKey()				{}
    explicit		    FullDBKey( const DBKey& dbky )
				: DBKey(dbky)			{}
    explicit		    FullDBKey( const SurveyDiskLocation& sdl )
				: survloc_(sdl)			{}
			    FullDBKey( const DBKey& dbky,
				       const SurveyDiskLocation& sdl )
				: DBKey(dbky), survloc_(sdl)	{}
			    FullDBKey( const SurveyDiskLocation& sdl,
				       const DBKey& dbky )
				: DBKey(dbky), survloc_(sdl)	{}
    FullDBKey&		    operator =(const DBKey&);
    FullDBKey&		    operator =(const FullDBKey&);
    bool		    operator ==(const FullDBKey&) const;
    bool		    operator !=(const FullDBKey&) const;
    bool		    operator ==(const DBKey&) const;
    bool		    operator !=(const DBKey&) const;

    SurveyDiskLocation	    survloc_;

    BufferString	    surveyName() const;

};
