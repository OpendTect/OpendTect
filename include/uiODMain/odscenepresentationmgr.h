#ifndef odscenemgr_h
#define odscenemgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "commondefs.h"
#include "odpresentationmgr.h"
#include "uistring.h"


mExpClass(uiODMain) ScenePresentationMgr : public ODVwrTypePresentationMgr
{ mODTextTranslationClass(ScenePresentationMgr);
public:
			ScenePresentationMgr();
    int			viewerTypeID()	{ return sViewerTypeID(); }
    static int		sViewerTypeID() { return 0; }

    static void		initClass();
};

#endif
