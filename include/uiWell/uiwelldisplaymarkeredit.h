#ifndef uiwelldisplaymarkeredit_h
#define uiwelldisplaymarkeredit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
 RCS:           $Id: uiwelldisplaymarkeredit.h,v 1.8 2012-04-23 13:13:11 cvsbruno Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"
#include "uidialog.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiMenuHandler;
class uiPushButton;
class uiToolButton;
class uiWellDisplayControl;

namespace Well { class Marker; class MarkerSet; class Data; }


mClass uiAddEditMrkrDlg : public uiDialog
{
public :
    				uiAddEditMrkrDlg(uiParent*,Well::Marker&);

    void			putToScreen();

protected :

    Well::Marker&		marker_;

    uiGenInput* 		namefld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			stratmrkfld_;

    bool			acceptOK(CallBacker*);
};



mClass uiWellDispEditMarkerDlg : public uiDialog
{
public:
				uiWellDispEditMarkerDlg(uiParent*);
				~uiWellDispEditMarkerDlg();

    void 			addWellCtrl(uiWellDisplayControl&,Well::Data&);

    bool 			isPicking() const 	{ return ispicking_; }
    bool 			needSave() const 	{ return needsave_; }

    Notifier<uiWellDispEditMarkerDlg> pickmodechanged;

protected:

    ObjectSet<uiWellDisplayControl> ctrls_;
    ObjectSet<Well::Data> 	wds_;

    uiWellDispEditMarkerDlg*	editdlg_;

    uiWellDisplayControl*	curctrl_;
    Well::Data*			curwd_;
    Well::Marker*		curmrk_;

    float			time_;
    float			dah_;

    uiToolButton*		pickbut_;
    uiToolButton*		rembut_;
    uiToolButton*		editbut_;
    uiToolButton*		addbut_;

    bool			needsave_;
    bool			ispicking_;

    ObjectSet<Well::MarkerSet>	orgmarkerssets_;
    uiGenInput*			modefld_;

    uiListBox*			mrklist_;
    ObjectSet<Well::Marker>	tobeadded_;
    TypeSet<Color>		colors_;

    bool 			hasedited_;
    bool 			ismarkerhit_;
    bool 			ispressed_;
    
    void			addNewMrkrList();
    void			editMrkrList();
    void			removeMrkrList();

    void			addMoveMarker();
    void			removeMarker();

    void			getMarkerFromAll(ObjectSet<Well::Marker>&,
	    					const char* nm); 

    void 			activateSensors(bool yn);
    void 			activateSensors(uiWellDisplayControl&,
						    Well::Data&,bool);

    bool			acceptOK(CallBacker*);
    void			buttonPushedCB(CallBacker*);
    void			editDlgClosedCB(CallBacker*);
    void                        editMarkerCB(CallBacker*);
    void			fillMarkerList(CallBacker*);
    void			handleUsrClickCB(CallBacker*);
    void			handleCtrlChangeCB(CallBacker*);
    void			modeChg(CallBacker*);
    void			listRClickCB(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			posChgCB(CallBacker*);
};

#endif
