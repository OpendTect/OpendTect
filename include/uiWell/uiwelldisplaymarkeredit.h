#ifndef uiwelldisplaymarkeredit_h
#define uiwelldisplaymarkeredit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
 RCS:           $Id: uiwelldisplaymarkeredit.h,v 1.12 2012-08-03 13:01:20 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwellmod.h"
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


mClass(uiWell) uiAddEditMrkrDlg : public uiDialog
{
public :
    				uiAddEditMrkrDlg(uiParent*,Well::Marker&,
							bool edit);

    void			putToScreen();

protected :

    Well::Marker&		marker_;

    uiGenInput* 		namefld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			stratmrkfld_;

    bool			acceptOK(CallBacker*);
};



mClass(uiWell) uiDispEditMarkerDlg : public uiDialog
{
public:
				uiDispEditMarkerDlg(uiParent*);
				~uiDispEditMarkerDlg();

    bool 			isPicking() const 	{ return ispicking_; }
    bool 			needSave() const 	{ return needsave_; }

    Notifier<uiDispEditMarkerDlg> pickmodechanged;

protected:

    Well::Marker*		curmrk_;

    uiToolButton*		pickbut_;
    uiToolButton*		rembut_;
    uiToolButton*		editbut_;
    uiToolButton*		addbut_;

    bool			needsave_;
    bool			ispicking_;

    ObjectSet<Well::MarkerSet>	orgmarkerssets_;
    uiGenInput*			modefld_;

    uiListBox*			mrklist_;
    ObjectSet<Well::Marker>	tmplist_;
    TypeSet<Color>		colors_;

    bool 			hasedited_;
    bool 			ismarkerhit_;
    bool 			ispressed_;
    
    virtual void		addNewMrkrList();
    virtual void		editMrkrList();
    virtual void		removeMrkrList();

    virtual void		addMoveMarker();
    virtual void		removeMarker();

    virtual void		getMarkerFromAll(ObjectSet<Well::Marker>&,
	    					const char* nm); 
    virtual Well::Marker*	getMarkerFromTmpList(const char* nm); 


    virtual bool		acceptOK(CallBacker*);
    void			buttonPushedCB(CallBacker*);
    virtual void		editDlgClosedCB(CallBacker*);
    virtual void		fillMarkerList(CallBacker*);
    virtual void		handleUsrClickCB(CallBacker*);
    virtual void		modeChg(CallBacker*);
    virtual void		listRClickCB(CallBacker*);
    virtual bool		rejectOK(CallBacker*);
};



mClass(uiWell) uiWellDispEditMarkerDlg : public uiDispEditMarkerDlg
{
public:
				uiWellDispEditMarkerDlg(uiParent*);

    void 			addWellCtrl(uiWellDisplayControl&,Well::Data&);

protected:

    ObjectSet<uiWellDisplayControl> ctrls_;
    ObjectSet<Well::Data> 	wds_;

    uiWellDisplayControl*	curctrl_;
    Well::Data*			curwd_;

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
    void			editDlgClosedCB(CallBacker*);
    void			fillMarkerList(CallBacker*);
    void			handleUsrClickCB(CallBacker*);
    void			handleCtrlChangeCB(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			posChgCB(CallBacker*);
};


#endif

