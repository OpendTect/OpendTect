#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.1 2007-05-23 17:05:28 cvsbert Exp $
-*/

#include "uidialog.h"
class CtxtIOObj;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiFileInput;
class uiListBox;
class uiPushButton;


class uiMadagascarMain : public uiDialog
{
public:

				uiMadagascarMain(uiParent*);
				~uiMadagascarMain();

protected:

    CtxtIOObj&			in3dctio_;
    CtxtIOObj&			in2dctio_;
    CtxtIOObj&			inpsctio_;
    CtxtIOObj&			out3dctio_;
    CtxtIOObj&			out2dctio_;
    CtxtIOObj&			outpsctio_;

    uiGenInput*			intypfld_;
    uiSeisSel*			inpseis3dfld_;
    uiSeisSel*			inpseis2dfld_;
    uiIOObjSel*			inpseispsfld_;
    uiFileInput*		inpmadfld_;
    uiGenInput*			outtypfld_;
    uiSeisSel*			outseis3dfld_;
    uiSeisSel*			outseis2dfld_;
    uiIOObjSel*			outseispsfld_;
    uiFileInput*		outmadfld_;

    uiListBox*			procsfld_;
    uiPushButton*		addbut_;
    uiPushButton*		editbut_;
    uiPushButton*		rmbut_;

    void			initWin(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			butPush(CallBacker*);
    void			typSel(CallBacker*);

    void			dispFlds(int,uiSeisSel*,uiSeisSel*,
	    				 uiIOObjSel*,uiFileInput*);
    void			setButStates();
    bool			ioOK(int,bool);

};


#endif
