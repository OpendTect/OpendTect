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
    void		setSelView(bool isnewsel = true );
    
protected:
    
    bool                manip_;
    
    uiToolBar*		toolbar_;
    uiToolButton*	manipdrawbut_;
    
    WellTiePickSetMGR*  picksetmgr_;
    
    bool 		checkIfInside(double,double);
    void 		finalPrepare();
    bool 		handleUserClick();
   
    void 		altZoomCB(CallBacker*);
    void 		editKeyPressCB(CallBacker*);
    void		rubBandCB(CallBacker*);
    void 		wheelMoveCB(CallBacker*);

};

#endif
