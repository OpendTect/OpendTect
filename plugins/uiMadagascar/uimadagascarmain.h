#ifndef uitutseistools_h
#define uitutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
 * ID       : $Id: uimadagascarmain.h,v 1.3 2007-06-12 10:24:46 cvsbert Exp $
-*/

#include "uidialog.h"
class CtxtIOObj;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiFileInput;
class uiListBox;
class uiPushButton;
class uiSeis2DSubSel;
class uiSeis3DSubSel;


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
    int				idx3d_, idx2d_, idxps_, idxmad_, idxnone_;

    uiGenInput*			intypfld_;
    uiSeisSel*			inpseis3dfld_;
    uiSeisSel*			inpseis2dfld_;
    uiIOObjSel*			inpseispsfld_;
    uiFileInput*		inpmadfld_;
    uiSeis3DSubSel*		subsel3dfld_;
    uiSeis2DSubSel*		subsel2dfld_;
    uiSeis3DSubSel*		subselpsfld_;

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
    void			inpSel(CallBacker*);

    void			dispFlds(int,uiSeisSel*,uiSeisSel*,
	    				 uiIOObjSel*,uiFileInput*);
    void			setButStates();
    bool			ioOK(int,bool);

};


#endif
