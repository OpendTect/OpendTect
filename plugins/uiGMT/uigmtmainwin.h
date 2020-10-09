#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "gmtpar.h"

#include "uidialog.h"

#include "netservice.h"

class CtxtIOObj;
class uiBatchJobDispatcherSel;
class uiGMTBaseMapGrp;
class uiGMTOverlayGrp;
class uiFileSel;
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

    uiFileSel*		filefld_;
    uiPushButton*	createbut_;
    uiPushButton*	viewbut_;

    uiPushButton*	addbut_;
    uiPushButton*	editbut_;
    uiPushButton*	resetbut_;

    uiTabStack*		tabstack_;
    ObjectSet<uiGMTOverlayGrp> overlaygrps_;

    ObjectSet<GMTPar>	pars_;
    bool		needsave_;

    TypeSet<Batch::ID>   procids_;
    TypeSet<Network::Service::ID>    servids_;
    BufferStringSet     outfnms_;
    uiBatchJobDispatcherSel*	batchfld_;

    void        postFinaliseCB( CallBacker* );
    void		createPushCB(CallBacker*);
    void        batchStartedCB(CallBacker*);
    void        batchEndedCB(CallBacker*);
    void		viewPush(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker*);
    void		selChg(CallBacker*);
    void		tabSel(CallBacker*);
    void		addCB(CallBacker*);
    void		editCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);

    bool		fillPar();
    bool		usePar(const IOPar&);

private:

    uiString		getCaptionStr() const;
};
