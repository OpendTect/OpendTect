#ifndef uiwelltiecontrolview_h
#define uiwelltiecontrolview_h

/*+
  ________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiflatviewstdcontrol.h"


class uiFlatViewer;
class uiButton;
class uiToolBar;
class uiIOObjSelDlg;
class uiRect;

namespace WellTie
{
    class DataHolder;
    class uiMrkDispDlg;

mClass uiControlView : public uiFlatViewStdControl
{
public:
			uiControlView(uiParent*,uiToolBar*,uiFlatViewer*);
			~uiControlView(){};
   
    const bool 		isZoomAtStart() const;
    void 		setEditOn(bool);
    void		setDataHolder(WellTie::DataHolder* dh)
    			{ dataholder_ = dh; }
    void		setSelView(bool isnewsel = true, bool viewall=false );
    
protected:
    
    bool                manip_;
    
    uiToolBar*		toolbar_;
    uiToolButton*	horbut_;
    uiToolButton*	hormrkdispbut_;
    uiIOObjSelDlg*	selhordlg_;
    uiWorldRect		curview_;
    
    WellTie::DataHolder* dataholder_;
    uiMrkDispDlg*	hormrkdispdlg_;
    
    bool 		checkIfInside(double,double);
    void 		finalPrepare();
    bool 		handleUserClick();
   
    void 		altZoomCB(CallBacker*);
    void 		keyPressCB(CallBacker*);
    void		loadHorizons(CallBacker*);
    void		dispHorMrks(CallBacker*);
    void		rubBandCB(CallBacker*);
    void 		wheelMoveCB(CallBacker*);

    friend class	uiTieWin;
};

/*
mClass uiTieClippingDlg : public uiDialog
{
public:
				uiTieClippingDlg(uiParent*);
				~uiTieClippingDlg();

protected :

    mStruct ClipData
    {
	float			cliprate_;			
	bool			issynthetic_;			
	Interval<float> 	timerange_;
    };

    uiGenInput*                 tracetypechoicefld_;
    uiSliderExtra*              sliderfld_;
    uiGenInput*                 timerangefld_;

    ClipData 			cd_;

    void 			getFromScreen(Callbacker*);
    void 			putToScreen(Callbacker*);
    void 			applyClipping(CallBacker*);
};
*/

}; //namespace WellTie

#endif
