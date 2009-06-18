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
class uiToolBar;
class uiToolButton;
class uiWellTieViewPropDlg;

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
    uiToolButton*       zoominbut_;
    uiToolButton*       zoomoutbut_;
    uiToolButton*       manipdrawbut_;
    uiToolButton*       disppropbut_;
    
    WellTiePickSetMGR*  picksetmgr_;
   
    bool 		checkIfInside(double,double);
    bool 		handleUserClick();
};

#endif
