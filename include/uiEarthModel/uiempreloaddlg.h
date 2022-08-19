#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiListBox;
class uiPushButton;
class uiTextEdit;
class uiToolButton;

mExpClass(uiEarthModel) uiEMPreLoadDlg : public uiDialog
{ mODTextTranslationClass(uiEMPreLoadDlg);
public:

protected:
			uiEMPreLoadDlg(uiParent*,const Setup&);

    virtual void	unloadPushCB(CallBacker*)		=0;
    virtual void	selCB(CallBacker*)			=0;

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;
    uiPushButton*	unloadbut_;
    uiToolButton*	savebut_;
};


mExpClass(uiEarthModel) uiHorizonPreLoadDlg : public uiEMPreLoadDlg
{ mODTextTranslationClass(uiHorizonPreLoadDlg);
public:
			uiHorizonPreLoadDlg(uiParent*);
protected:
    void		add3DPushCB(CallBacker*);
    void		add2DPushCB(CallBacker*);
    bool		loadHorizon(bool);
    void		unloadPushCB(CallBacker*) override;
    void		selCB(CallBacker*) override;
    void		openPushCB(CallBacker*);
    void		savePushCB(CallBacker*);
    void		loadSavedHorizon(const TypeSet<MultiID>&);
};
