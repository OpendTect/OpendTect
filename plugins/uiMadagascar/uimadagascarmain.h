#ifndef uimadagascarmain_h
#define uimadagascarmain_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.11 2007-12-11 15:18:26 cvsbert Exp $
-*/

#include "uidialog.h"
#include "iopar.h"
class uiGroup;
class uiMadIOSel;
class uiListBox;
class uiToolButton;
class uiMadagascarBldCmd;


class uiMadagascarMain : public uiDialog
{
public:

			uiMadagascarMain(uiParent*);

protected:

    IOPar		iniop_;
    IOPar		outiop_;

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
    void		selChg(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);
    void		importFlow(CallBacker*);
    void		exportFlow(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		createMenus();
    uiGroup*		crProcGroup(uiGroup*);

};


#endif
