#ifndef uiwelldisplaymarkeredit_h
#define uiwelldisplaymarkeredit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
 RCS:           $Id: uiwelldisplaymarkeredit.h,v 1.4 2010-10-01 17:12:18 cvsbruno Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"
#include "uidialog.h"
#include "uigroup.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiMenuHandler;
class uiPushButton;
class uiWellDisplayControl;

namespace Well { class Marker; class MarkerSet; class Data; }


mStruct WellDispMarkerParams
{
				WellDispMarkerParams()
				    : dah_(0)
				    {}

    float 			time_;
    float 			dah_;
    Color               	col_;
    BufferString        	name_;
    bool                	isstrat_;

    void			getFromMarker(const Well::Marker&);
    void			putToMarker(Well::Marker&);
};


mClass uiWellDispMarkerEditGrp : public uiGroup
{
public :
    				uiWellDispMarkerEditGrp(uiParent*,
							WellDispMarkerParams&);

    bool			checkPars();
    void			setDefault();
    void			setFldsSensitive(bool yn);
    void			putToScreen();

    void			setPos(float,float);

    Notifier<uiWellDispMarkerEditGrp> dispparchg;

protected :

    WellDispMarkerParams&	par_;

    bool			istime_;

    void			getFromScreen(CallBacker*);

    uiGenInput* 		modefld_;
    uiGenInput* 		namefld_;
    uiGenInput*			posfld_;
    uiColorInput*		colorfld_;
    uiCheckBox*			stratmrkfld_;
};



mClass uiWellDispEditMarkerDlg : public uiDialog
{
public:
				uiWellDispEditMarkerDlg(uiParent*);
				~uiWellDispEditMarkerDlg();


    uiWellDispMarkerEditGrp&	grp() 			{ return *mrkgrp_; }

    void			setMode(bool);
    bool 			isAddRemMode() const;

    void 			addWellCtrl(uiWellDisplayControl&,Well::Data&);
    
    void			startEdit();

    bool 			needSave() 	{ return needsave_; }

protected:

    ObjectSet<uiWellDisplayControl> ctrls_;
    ObjectSet<Well::Data> 	wds_;

    uiWellDispEditMarkerDlg*	editdlg_;
    WellDispMarkerParams	par_;

    uiWellDisplayControl*	curctrl_;
    Well::Data*			curwd_;
    Well::Marker*		curmrk_;

    bool			needsave_;

    ObjectSet<Well::MarkerSet>	orgmarkerssets_;
    uiWellDispMarkerEditGrp*	mrkgrp_;
    uiGenInput*			modefld_;

    uiListBox*			mrklist_;
    ObjectSet<Well::Marker>	tmplist_;
    TypeSet<Color>		colors_;

    bool 			hasedited_;
    bool 			ismarkerhit_;
    bool 			ispressed_;
    
    void			addNewMarker();
    void                        changeMarkerPos(Well::Marker&);
    void			handleEditMarker();
    void			removeMarker();
    void			setParsFromMarker(const Well::Marker&);
    void			setParsToMarker(Well::Marker&);

    void 			activateSensors(bool yn);
    void 			activateSensors(uiWellDisplayControl&,
						    Well::Data&,bool);

    void			modeChg(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			editDlgClosedCB(CallBacker*);
     void                       editMarkerCB(CallBacker*);
    void			handleUsrClickCB(CallBacker*);
    void			handleCtrlChangeCB(CallBacker*);
    void			posChgCB(CallBacker*);

    void			listLClickCB();
    void			listRClickCB();
    void			fillMarkerList(CallBacker*);
};

#endif
