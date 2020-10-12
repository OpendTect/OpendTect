#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "multiid.h"

class SurveyDiskLocation;

mExpClass(Basic) DBKey : public MultiID
{
public:
			DBKey()
			{}
			DBKey( const char* keystr )
			    : MultiID(keystr)
			{}
			DBKey( const MultiID& mid )
			    : MultiID(mid)
			{}
			~DBKey();

    bool		isValid() const		{ return !isUdf(); }

    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    bool		hasSurveyLocation() const { return survloc_; }
    const SurveyDiskLocation& surveyDiskLocation() const;
    void		clearSurveyDiskLocation();

    BufferString	toString(bool withsurvloc) const;

protected:

    SurveyDiskLocation*		survloc_ = nullptr;
};
