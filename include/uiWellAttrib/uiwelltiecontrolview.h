#ifndef uiwelltiecontrolview_h
#define uiwelltiecontrolview_h

/*+
  ________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"

class WellTieDisplayProperties;

class uiFlatViewer;
class uiToolBar;
class uiToolButton;
class uiWellTieViewPropDlg;
class WellTiePickSetManager;

mClass uiWellTieControlView : public uiFlatViewControl
{
public:
			uiWellTieControlView(uiParent*,uiToolBar*,
			    ObjectSet<uiFlatViewer>&,WellTiePickSetManager&);
			~uiWellTieControlView();
   
    void 		setView();
    
protected:
    
    bool                manip_;
    
    uiToolBar*		toolbar_;
    uiToolButton*       zoominbut_;
    uiToolButton*       zoomoutbut_;
    uiToolButton*       manipdrawbut_;
    uiToolButton*       disppropbut_;
    uiWellTieViewPropDlg* propdlg_;

    WellTieDisplayProperties* dprops_;
    WellTiePickSetManager& picksetmgr_;
    
    void		doPropDlg(CallBacker*);
    bool 		handleUserClick(const int);
    void		updateButtons();
    void		propDlgClosed(CallBacker*);
    void		rubBandCB(CallBacker*);
    void 		usrClickCB(CallBacker*);
    void 		mouseMoveCB(CallBacker*);
    void 		stateCB(CallBacker*);
    void		updateButtons(CallBacker*);
    void 		vwChgCB(CallBacker*);
    void		zoomOutCB(CallBacker*);
    void		zoomInCB(CallBacker*);

};

#endif
