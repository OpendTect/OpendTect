#ifndef odviewer2dmgr_h
#define odviewer2dmgr_h

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


mExpClass(uiODMain) Viewer2DPresentationMgr : public ODVwrTypePresentationMgr
{ mODTextTranslationClass(Viewer2DPresentationMgr);
public:
			Viewer2DPresentationMgr();
    int			viewerTypeID()	{ return sViewerTypeID(); }
    static int		sViewerTypeID() { return 1; }

    static void		initClass();
};

#endif
