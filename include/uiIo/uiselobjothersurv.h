#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016 / July 2018 (RIP)
________________________________________________________________________

-*/

#include "uisurvioobjseldlg.h"


/*!\brief get a single IOObj from an other survey */

mExpClass(uiIo) uiSelObjFromOtherSurvey : public uiSurvIOObjSelDlg
{
public:

    mDeprecated		uiSelObjFromOtherSurvey( uiParent* p,
						 const IOObjContext& ctxt )
			    : uiSurvIOObjSelDlg(p,ctxt)	{}

    mDeprecated BufferString	fullUserExpression() const
			{ return mainFileName(); }
    mDeprecated BufferString sourceSurveyDirectory() const
			{ return surveyDiskLocation().fullPath(); }

};
