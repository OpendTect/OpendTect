#ifndef uimadagascarmain_h
#define uimadagascarmain_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id$
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
{
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
    bool		saveFlow(CallBacker*);
    void		exportFlow(CallBacker*);
    bool		rejectOK(CallBacker*);

    bool		fillPar();
    void		createToolBar();
    void		updateCaption();
    uiGroup*		crProcGroup(uiGroup*);
    bool		acceptOK(CallBacker*);
};


#endif
