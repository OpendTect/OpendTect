#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "menuhandler.h"
#include "uidialog.h"
#include "uistring.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiMenuHandler;
class uiToolButton;
class uiWellDisplayControl;

namespace Well { class Marker; class MarkerSet; class Data; }


mExpClass(uiWell) uiAddEditMrkrDlg : public uiDialog
{ mODTextTranslationClass(uiAddEditMrkrDlg);
public :
				uiAddEditMrkrDlg(uiParent*,Well::Marker&,
							bool edit);
				~uiAddEditMrkrDlg();

    void			putToScreen();

protected :

    Well::Marker&		marker_;

    uiGenInput*			namefld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			stratmrkfld_;

    bool			acceptOK(CallBacker*) override;
};



mExpClass(uiWell) uiDispEditMarkerDlg : public uiDialog
{ mODTextTranslationClass(uiDispEditMarkerDlg);
public:
				uiDispEditMarkerDlg(uiParent*);
				~uiDispEditMarkerDlg();

    bool			isPicking() const	{ return ispicking_; }
    bool			needSave() const	{ return needsave_; }
    void			addMarkerSet(Well::MarkerSet&);

    Notifier<uiDispEditMarkerDlg> pickmodechanged;

protected:

    Well::Marker*		curmrk_;

    uiGroup*			toolgrp_;
    uiToolButton*		pickbut_;
    uiToolButton*		rembut_;
    uiToolButton*		editbut_;
    uiToolButton*		addbut_;

    bool			needsave_;
    bool			ispicking_;
    bool			hasedited_;

    ObjectSet<Well::MarkerSet>	orgmarkerssets_;
    ObjectSet<Well::MarkerSet>	markerssets_;

    uiListBox*			mrklist_;
    ObjectSet<Well::Marker>	tmplist_;
    TypeSet<OD::Color>		colors_;

    virtual void		addNewMrkrList();
    virtual void		editMrkrList();
    virtual bool		removeMrkrFromList();

    void			addMoveMarker(int,float,const char*);
    void			removeMarker(int,const char*);

    virtual void		getMarkerFromAll(ObjectSet<Well::Marker>&,
						const char* nm);
    virtual Well::Marker*	getMarkerFromTmpList(const char* nm); 


    bool			acceptOK(CallBacker*) override;
    void			buttonPushedCB(CallBacker*);
    virtual void		fillMarkerList(CallBacker*);
    virtual void		handleUsrClickCB(CallBacker*) =0;
    virtual void		modeChg(CallBacker*);
    virtual void		listRClickCB(CallBacker*);
    bool			rejectOK(CallBacker*) override;
};



mExpClass(uiWell) uiWellDispCtrlEditMarkerDlg : public uiDispEditMarkerDlg
{ mODTextTranslationClass(uiWellDispCtrlEditMarkerDlg);
public:
				uiWellDispCtrlEditMarkerDlg(uiParent*);
				~uiWellDispCtrlEditMarkerDlg();

    void			addWellCtrl(uiWellDisplayControl&,Well::Data&);
    void			askForSavingEditedChanges();

protected:

    ObjectSet<uiWellDisplayControl> ctrls_;
    ObjectSet<Well::Data>	wds_;

    uiWellDisplayControl*	curctrl_;
    Well::Data*			curwd_;

    void			editMrkrList() override;
    bool			removeMrkrFromList() override;

    void			triggerWDsMarkerChanged();
    bool			acceptOK(CallBacker*) override;
    void			handleUsrClickCB(CallBacker*) override;
    void			handleCtrlChangeCB(CallBacker*);
    bool			rejectOK(CallBacker*) override;
};
