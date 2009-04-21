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

class Color;
class UserPicks;
class WellTieDisplayProperties;

class uiFlatViewer;
class uiToolBar;
class uiToolButton;
class uiWellTieViewPropDlg;

mClass uiWellTieControlView : public uiFlatViewControl
{
public:
			uiWellTieControlView(uiParent*,uiToolBar* toolbar,
			       		       	ObjectSet<uiFlatViewer>&);
			~uiWellTieControlView();
   

    Notifier<uiWellTieControlView> seisPickPosAdded; 
    Notifier<uiWellTieControlView> synthPickPosAdded; 

    void 		setView();
    void		setUserPicks(UserPicks*,UserPicks*);

protected:
    
    bool                manip_;
    UserPicks*		synthpicks_;		
    UserPicks*		seispicks_;		

    uiToolBar*		toolbar_;
    uiToolButton*       zoominbut_;
    uiToolButton*       zoomoutbut_;
    uiToolButton*       manipdrawbut_;
    uiToolButton*       disppropbut_;
    uiWellTieViewPropDlg* propdlg_;
    WellTieDisplayProperties* dprops_;
    
    void		addPickPos(const int,const float);
    void		doPropDlg(CallBacker*);
    bool 		handleUserClick(const int);
    void		updateButtons();
    void		propDlgClosed(CallBacker*);
    void		rubBandCB(CallBacker*);
    void 		usrClickCB(CallBacker*);
    void 		stateCB(CallBacker*);
    void		updateButtons(CallBacker*);
    void 		vwChgCB(CallBacker*);
    void		zoomOutCB(CallBacker*);
    void		zoomInCB(CallBacker*);

};


mClass UserPicks
{
public:

			UserPicks()
		 	{}		    

	Color     	color_;
	int 		vieweridx_;
	TypeSet<float> 	zpos_;

};

#endif
