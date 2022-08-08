#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
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


