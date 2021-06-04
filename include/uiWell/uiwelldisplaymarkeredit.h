#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Aug 2010
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

    void			putToScreen();

protected :

    Well::Marker&		marker_;

    uiGenInput*			namefld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			stratmrkfld_;

    bool			acceptOK(CallBacker*);
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


    virtual bool		acceptOK(CallBacker*);
    void			buttonPushedCB(CallBacker*);
    virtual void		fillMarkerList(CallBacker*);
    virtual void		handleUsrClickCB(CallBacker*) =0;
    virtual void		modeChg(CallBacker*);
    virtual void		listRClickCB(CallBacker*);
    virtual bool		rejectOK(CallBacker*);
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

    void			editMrkrList();
    bool			removeMrkrFromList();

    void			triggerWDsMarkerChanged();
    bool			acceptOK(CallBacker*);
    void			handleUsrClickCB(CallBacker*);
    void			handleCtrlChangeCB(CallBacker*);
    bool			rejectOK(CallBacker*);
};


