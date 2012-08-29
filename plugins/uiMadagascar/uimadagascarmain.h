#ifndef uimadagascarmain_h
#define uimadagascarmain_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.21 2012-08-29 08:18:06 cvskris Exp $
-*/

#include "uimadagascarmod.h"
#include "uibatchlaunch.h"
#include "iopar.h"
#include "madprocflow.h"
class CtxtIOObj;
class uiGroup;
class uiMadIOSel;
class uiListBox;
class uiToolButton;
class uiMadagascarBldCmd;


class uiMadagascarMain : public uiFullBatchDialog
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

    bool		prepareProcessing()	{ return true; }
    bool		fillPar(IOPar&);
    void		createToolBar();
    void		updateCaption();
    uiGroup*		crProcGroup(uiGroup*);

};


#endif
