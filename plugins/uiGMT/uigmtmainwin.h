#ifndef uigmtmainwin_h
#define uigmtmainwin_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtmainwin.h,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"
#include "uibatchlaunch.h"

class Timer;
class uiGMTBaseMapGrp;
class uiGMTOverlayGrp;
class uiFileInput;
class uiListBox;
class uiPushButton;
class uiToolButton;
class uiTabStack;

class uiGMTMainWin : public uiFullBatchDialog
{
public:
    			uiGMTMainWin(uiParent*);
			~uiGMTMainWin();

protected:

    uiGMTBaseMapGrp*	basemapgrp_;
    uiGroup*		flowgrp_;
    uiListBox*		flowfld_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiToolButton*	rmbut_;

    uiFileInput*	filefld_;
    uiPushButton*	createbut_;
    uiPushButton*	viewbut_;

    uiPushButton*	addbut_;
    uiPushButton*	editbut_;

    uiTabStack*		tabstack_;
    ObjectSet<uiGMTOverlayGrp> overlaygrps_;

    ObjectSet<GMTPar>	pars_;
    Timer*		tim_;

    void		createPush(CallBacker*);
    void		viewPush(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker*);
    void		selChg(CallBacker*);
    void		tabSel(CallBacker*);
    void		addCB(CallBacker*);
    void		editCB(CallBacker*);
    void		checkFileCB(CallBacker*);

    bool		prepareProcessing()		{ return true; }
    bool		fillPar(IOPar&);
    void		makeLegendPar(IOPar&) const;
};

#endif
