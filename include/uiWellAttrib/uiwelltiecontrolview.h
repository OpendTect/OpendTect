#ifndef uiwelltiecontrolview_h
#define uiwelltiecontrolview_h

/*+
  ________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiflatviewstdcontrol.h"
#include "welltiepickset.h"

class WellTieDisplayProperties;

class uiFlatViewer;
class uiButton;
class uiToolBar;

mClass uiWellTieControlView : public uiFlatViewStdControl
{
public:
			uiWellTieControlView(uiParent*,uiToolBar*,
			    		     uiFlatViewer*);
			~uiWellTieControlView(){};
   
    void		setPickSetMGR(WellTiePickSetMGR* pmgr)
    			{ picksetmgr_ = pmgr; }
    
protected:
    
    bool                manip_;
    
    uiToolBar*		toolbar_;
    uiToolButton*	manipdrawbut_;
    
    WellTiePickSetMGR*  picksetmgr_;
   
    void 		altZoomCB(CallBacker*);
    bool 		checkIfInside(double,double);
    bool 		handleUserClick();
    void		rubBandCB(CallBacker*);
    void 		stateCB(CallBacker*);

};

#endif
