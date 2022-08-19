#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimadagascarmod.h"
#include "uidialog.h"
#include "iopar.h"
#include "madprocflow.h"

class CtxtIOObj;
class uiGroup;

class uiBatchJobDispatcherSel;
class uiMadIOSel;
class uiListBox;
class uiToolButton;
class uiMadagascarBldCmd;


mClass(uiMadagascar) uiMadagascarMain : public uiDialog
{ mODTextTranslationClass(uiMadagascarMain);
public:

			uiMadagascarMain(uiParent*);
			~uiMadagascarMain();

    bool		askSave(bool withcancel=true);
    Notifier<uiMadagascarMain> windowHide;

protected:

    CtxtIOObj&		ctio_;
    bool		needsave_;

    uiMadIOSel*		infld_;
    uiMadIOSel*		outfld_;
    uiListBox*		procsfld_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiToolButton*	rmbut_;
    uiMadagascarBldCmd*	bldfld_;
    uiBatchJobDispatcherSel* batchfld_;

    ODMad::ProcFlow&	procflow_;

    void		cmdAvail(CallBacker*);
    void		hideReq(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker* cb=0);
    void		inpSel(CallBacker*);
    void		selChg(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlowCB(CallBacker*);
    void		exportFlow(CallBacker*);
    bool		rejectOK(CallBacker*);

    bool		saveFlow();
    bool		fillPar();
    void		createToolBar();
    void		updateCaption();
    uiGroup*		crProcGroup(uiGroup*);
    bool		acceptOK(CallBacker*);
};
