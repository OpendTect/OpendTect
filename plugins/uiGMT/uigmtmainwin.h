#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtpar.h"
#include "uidialog.h"

class CtxtIOObj;
class Timer;
class uiBatchJobDispatcherSel;
class uiGMTBaseMapGrp;
class uiGMTOverlayGrp;
class uiFileInput;
class uiListBox;
class uiPushButton;
class uiToolButton;
class uiTabStack;

mClass(uiGMT) uiGMTMainWin : public uiDialog
{ mODTextTranslationClass(uiGMTMainWin);
public:
			uiGMTMainWin(uiParent*);
			~uiGMTMainWin();

protected:

    CtxtIOObj&		ctio_;
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
    uiPushButton*	resetbut_;

    uiTabStack*		tabstack_;
    ObjectSet<uiGMTOverlayGrp> overlaygrps_;

    ObjectSet<GMTPar>	pars_;
    Timer*		tim_;
    bool		needsave_;
    uiBatchJobDispatcherSel*	batchfld_;

    void		createPush(CallBacker*);
    void		viewPush(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker*);
    void		selChg(CallBacker*);
    void		tabSel(CallBacker*);
    void		addCB(CallBacker*);
    void		editCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		checkFileCB(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		fillPar();
    bool		usePar( const IOPar&);

private:

    uiString		getCaptionStr() const;
};
