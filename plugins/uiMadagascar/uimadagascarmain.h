#ifndef uimadagascarmain_h
#define uimadagascarmain_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.14 2008-05-09 13:09:37 cvsraman Exp $
-*/

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

    bool		getFlow(ODMad::ProcFlow&);
    void		setFlow(const ODMad::ProcFlow&);

protected:

    CtxtIOObj&		ctio_;

    uiMadIOSel*		infld_;
    uiMadIOSel*		outfld_;
    uiListBox*		procsfld_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiToolButton*	rmbut_;
    uiMadagascarBldCmd*	bldfld_;

    void		cmdAvail(CallBacker*);
    void		hideReq(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker* cb=0);
    void		inpSel(CallBacker*);
    void		selChg(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);
    void		exportFlow(CallBacker*);

    bool		prepareProcessing()	{ return true; }
    bool		fillPar(IOPar&);
    void		createMenus();
    uiGroup*		crProcGroup(uiGroup*);

};


#endif
