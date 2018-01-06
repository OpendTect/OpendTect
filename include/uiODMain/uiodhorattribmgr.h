#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uivismenuitemhandler.h"
class uiODMain;
class uiEMDataPointSetPickDlg;


mExpClass(uiODMain) uiODHorAttribMgr :  public CallBacker
{ mODTextTranslationClass(uiODHorAttribMgr)
public:

			uiODHorAttribMgr(uiODMain*);
			~uiODHorAttribMgr();

    void		updateMenu(CallBacker*);
    void		makeStratAmp(CallBacker*);
    void		doIsochron(CallBacker*);
    void		doIsochronThruMenu(CallBacker*);
    void		doContours(CallBacker*);
    void		calcPolyVol(CallBacker*);
    void		calcHorVol(CallBacker*);
    void		pickData(CallBacker*);
    void		dataReadyCB(CallBacker*);

    uiVisMenuItemHandler isochronmnuitemhndlr_;
    uiVisMenuItemHandler contourmnuitemhndlr_;
    uiVisMenuItemHandler horvolmnuitemhndlr_;
    uiVisMenuItemHandler pickdatamnuitemhndlr_;
    uiPickSetPolygonMenuItemHandler polyvolmnuitemhndlr_;

    uiODMain*			appl_;
    uiEMDataPointSetPickDlg*	dpspickdlg_;
};
