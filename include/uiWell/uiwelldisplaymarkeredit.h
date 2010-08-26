#ifndef uiwelldisplaymarkeredit_h
#define uiwelldisplaymarkeredit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
 RCS:           $Id: uiwelldisplaymarkeredit.h,v 1.2 2010-08-26 14:37:28 cvsbruno Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"
#include "uidialog.h"
#include "uigroup.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
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
				uiWellDispEditMarkerDlg(uiParent*,
						WellDispMarkerParams&);
				~uiWellDispEditMarkerDlg(){};


    uiWellDispMarkerEditGrp&	grp() 			{ return *mrkgrp_; }

    void			setMode(bool);
    bool 			isAddRemMode() const;

protected:

    uiWellDispMarkerEditGrp*	mrkgrp_;
    uiGenInput*			modefld_;

    void			modeChg(CallBacker*);
};



mClass uiWellDispMarkerEditor : public CallBacker
{
public:
				uiWellDispMarkerEditor(uiParent*); 
				~uiWellDispMarkerEditor();

    void 			addCtrl(uiWellDisplayControl&,Well::Data&);
    void 			removeCtrl(uiWellDisplayControl&,Well::Data&);

    Notifier<uiWellDispMarkerEditor> editFinished;

protected:

    ObjectSet<uiWellDisplayControl> ctrls_;
    ObjectSet<Well::Data> 	wds_;

    uiWellDispEditMarkerDlg*	editdlg_;
    WellDispMarkerParams	par_;

    uiWellDisplayControl*	curctrl_;
    Well::Data*			curwd_;
    Well::Marker*		curmrk_;

    Well::Data*			lasteditwd_;
    Well::Marker*		lasteditmrk_;

    ObjectSet<Well::Marker>	addedmarkers_;

    bool 			isediting_;
    bool 			ismarkerhit_;
    bool 			ispressed_;
    
    uiMenuHandler*		menu_;
    MenuItem            	startmnuitem_;

    void                        createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    void                        changeMarkerPos(Well::Marker&);

    void			handleMenuMaker();
    void			handleEditMarker();

    void			addNewMarker();
    void			removeMarker();
    void			setParsFromMarker(const Well::Marker&);
    void			setParsToMarker(Well::Marker&);

    void			editDlgClosedCB(CallBacker*);

    void			posChgCB(CallBacker*);
    void			handleUsrClickCB(CallBacker*);
    void			handleCtrlChangeCB(CallBacker*);
    void                        editMarkerCB(CallBacker*);
};

#endif
