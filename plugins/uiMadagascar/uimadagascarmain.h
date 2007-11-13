#ifndef uimadagascarmain_h
#define uimadagascarmain_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.9 2007-11-13 16:21:27 cvsbert Exp $
-*/

#include "uidialog.h"
#include "iopar.h"
class uiMadIOSel;
class uiListBox;
class uiToolButton;


class uiMadagascarMain : public uiDialog
{
public:

			uiMadagascarMain(uiParent*);
			~uiMadagascarMain();

protected:

    IOPar		iniop_;
    IOPar		outiop_;

    uiMadIOSel*		infld_;
    uiMadIOSel*		outfld_;
    uiListBox*		procsfld_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiToolButton*	rmbut_;

    bool		acceptOK(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker* cb=0);
    void		selChg(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);
    void		importFlow(CallBacker*);
    void		exportFlow(CallBacker*);

    void		createMenus();
    uiGroup*		crProcGroup();

};


#endif
