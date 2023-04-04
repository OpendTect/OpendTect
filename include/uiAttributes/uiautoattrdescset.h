#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "bufstringset.h"

class uiAttrDescEd;
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiIOObjSelGrp;
class CtxtIOObj;
class IOObj;


/*!
\brief Class for selecting Auto-load Attribute Set.
*/

mExpClass(uiAttributes) uiAutoAttrSelDlg : public uiDialog
{ mODTextTranslationClass(uiAutoAttrSelDlg);
public:
    			uiAutoAttrSelDlg(uiParent* p,bool);
			~uiAutoAttrSelDlg();

    IOObj*		getObj();
    bool		useAuto();
    bool		loadAuto();

protected:

    CtxtIOObj&		ctio_;
    bool		is2d_;

    uiGenInput*		usefld_;
    uiIOObjSelGrp*	selgrp_;
    uiLabel*		lbl_;
    uiCheckBox*		loadbutton_;

    void		useChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};


mExpClass(uiAttributes) uiAutoAttrSetOpen : public uiDialog
{ mODTextTranslationClass(uiAutoAttrSetOpen);
public:
			uiAutoAttrSetOpen(uiParent*,BufferStringSet&,
					BufferStringSet&);
			~uiAutoAttrSetOpen();

    IOObj*		getObj();
    const char*		getAttribname();
    const char*		getAttribfile();
    bool		isUserDef()		{ return usrdef_; }
    bool		isAuto()		{ return isauto_; }

protected:

    CtxtIOObj&		ctio_;

    uiIOObjSelGrp*	selgrp_;
    uiLabel*		lbl_;
    uiListBox*		defattrlist_;
    uiGenInput*		defselfld_;
    uiGenInput*		autoloadfld_;

    BufferStringSet	attribfiles_;
    BufferStringSet	attribnames_;
    int			defselid_;
    bool		usrdef_;
    bool		isauto_;

    void		setChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
